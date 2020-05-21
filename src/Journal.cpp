/**********************************************************************

  Audacity: A Digital Audio Editor

  Journal.cpp

  Paul Licameli

*******************************************************************//*!

\namespace Journal
\brief Facilities for recording and playback of sequences of user interaction

*//*******************************************************************/

#include "Journal.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <wx/app.h>
#include <wx/event.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/toplevel.h>

#include "MemoryX.h"
#include "Prefs.h"

namespace {

/******************************************************************************
utilities to identify corresponding windows between recording and playback runs
******************************************************************************/

using WindowPath = Identifier;
constexpr auto WindowPathSeparator = ':';
constexpr auto WindowPathEscape = '\\';

// Is the window uniquely named among top level windows or among children
// of its parent?
bool HasUniqueNameAmongPeers( wxWindow &window, const wxWindowList &list )
{
   auto name = window.GetName();
   auto pred = [&name]( wxWindow *pWindow2 ){
      return pWindow2->GetName() == name;
   };
   auto begin = wxTopLevelWindows.begin(),
      end = wxTopLevelWindows.end(), iter1 = begin;
   for ( ; iter1 != end; ++iter1 )
      if ( pred( *iter1 ) )
         break;
   auto iter2 = iter1;
   for ( ; iter2 != end; ++iter2 )
      if ( pred( *iter2 ) )
         break;
   return ( iter2 == end && iter1 != end && *iter1 == &window );
}

// Find the unique window in the list for the given name, if there is such
wxWindow *FindWindowByNameAmongPeers(
   const wxString &name, const wxWindowList &list )
{
   auto pred = [&name]( wxWindow *pWindow2 ){
      return pWindow2->GetName() == name;
   };
   auto begin = wxTopLevelWindows.begin(),
      end = wxTopLevelWindows.end(), iter1 = begin;
   for ( ; iter1 != end; ++iter1 )
      if ( pred( *iter1 ) )
         break;
   auto iter2 = iter1;
   for ( ; iter2 != end; ++iter2 )
      if ( pred( *iter2 ) )
         break;
   if ( iter2 == end && iter1 != end )
      return *iter1;
   return nullptr;
}

// Find array of window names, starting with a top-level window and ending
// with the given window, or an empty array of the conditions fail for
// uniqueness of names.
void WindowPathComponents( wxWindow &window, wxArrayStringEx &components )
{
   if ( dynamic_cast<wxTopLevelWindow*>( &window ) ) {
      if ( HasUniqueNameAmongPeers( window, wxTopLevelWindows ) ) {
         components.push_back( window.GetName() );
         return;
      }
   }
   else if ( auto pParent = window.GetParent() ) {
      // Recur
      WindowPathComponents( *pParent, components );
      if ( !components.empty() &&
          HasUniqueNameAmongPeers( window, pParent->GetChildren() ) ) {
         components.push_back( window.GetName() );
         return;
      }
   }

   // Failure
   components.clear();
}

// When recording, find a string to identify the window in the journal
WindowPath FindWindowPath( wxWindow &window )
{
   wxArrayStringEx components;
   WindowPathComponents( window, components );
   return wxJoin( components, WindowPathSeparator, WindowPathEscape );
}

// When playing, find a window by path, corresponding to the window that had the
// same path in a previous run
wxWindow *FindWindowByPath( const WindowPath &path )
{
   wxArrayStringEx components;
   wxSplit( path.GET(), WindowPathSeparator, WindowPathEscape );
   if ( !components.empty() ) {
      auto iter = components.begin(), end = components.end();
      wxWindow *pWindow =
         FindWindowByNameAmongPeers( *iter++, wxTopLevelWindows );
      while ( pWindow && iter != end )
         pWindow = FindWindowByNameAmongPeers( *iter++, pWindow->GetChildren() );
      return pWindow;
   }
   return nullptr;
}

/******************************************************************************
utilities to record events to journal and recreate them on playback
******************************************************************************/

// Events need to be recorded in the journal, but the numbers associated with
// event types by wxWidgets are chosen dynamically and may not be the same
// across runs or platforms.  We need an invariant name for each event type
// of interest, which also makes the journal more legible than if we wrote mere
// numbers.
using EventCode = Identifier;

// An entry in a catalog describing the types of events that are intercepted
// and recorded, and simulated when playing back.
struct EventType {
   // Function that returns a list of parameters that, with the event type,
   // are sufficient to record an event to the journal and recreate it on
   // playback
   using Serializer = std::function< wxArrayStringEx( const wxEvent& ) >;

   // Function that recreates an event at playback
   using Deserializer =
      std::function< std::unique_ptr<wxEvent>( const wxArrayStringEx& ) >;

   wxEventType type;
   EventCode code;
   Serializer serializer;
   Deserializer deserializer;

   // Type-erasing constructor so you can avoid casting when you supply
   // the functions
   template<
      typename Tag, // such as wxCommandEvent
      typename SerialFn,
      typename DeserialFn >
   EventType( wxEventTypeTag<Tag> type, // such as wxEVT_BUTTON
      const EventCode &code,
      const SerialFn &serialFn, const DeserialFn &deserialFn )
      : type{ type }
      , code{ code }
      , serializer{ [serialFn]( const wxEvent &e )
         { return serialFn(static_cast<const Tag&>(e)); } }
      , deserializer{ [deserialFn]( const wxArrayStringEx &strings )
         -> std::unique_ptr< wxEvent >
         { return deserialFn(strings); } }
   {
      // Check consistency of the deduced template parameters
      // The deserializer should produce unique pointer to Tag
      using DeserializerResult = decltype( *deserialFn( wxArrayStringEx{} ) );
      static_assert( std::is_same< Tag&, DeserializerResult >::value,
         "deserializer produces the wrong type" );
   }
};
using EventTypes = std::vector< EventType >;

// The list of event types to intercept and record.  Its construction must be
// delayed until wxWidgets has initialized and chosen the integer values of
// event types.
static const EventTypes &typeCatalog()
{
   static EventTypes result {
      { wxEVT_BUTTON, "Press",
         [](const auto &event){ return wxArrayString{}; },
         [](const wxArrayStringEx &){
            return std::make_unique<wxCommandEvent>(); } },
   };
   return result;
}

// Lookup into the event type catalog during recording
using ByType = std::map< wxEventType, const EventType& >;
static const ByType &byType()
{
   static std::once_flag flag;
   static ByType result;
   std::call_once( flag, []{
      for ( const auto &type : typeCatalog() )
         result.emplace( type.type, type );
   } );
   return result;
}

// Lookup into the event type catalog during playback
using ByCode = std::unordered_map< EventCode, const EventType & >;
static const ByCode &byCode()
{
   static std::once_flag flag;
   static ByCode result;
   std::call_once( flag, []{
      for ( const auto &type : typeCatalog() )
         result.emplace( type.code, type );
   } );
   return result;
}

constexpr auto SeparatorCharacter = ',';
constexpr auto CommentCharacter = '#';

wxString sFileNameIn;
wxTextFile sFileIn;

wxString sLine;
// Invariant:  the input file has not been opened, or else sLineNumber counts
// the number of lines consumed by the tokenizer
int sLineNumber = -1;

struct FlushingTextFile : wxTextFile {
   // Flush output when the program quits, even if that makes an incomplete
   // journal file without an exit
   ~FlushingTextFile() { if ( IsOpened() ) { Write(); Close(); } }
} sFileOut;

constexpr auto EnabledKey = wxT("/Journal/Enabled");
bool sRecordEnabled = false;

bool sError = false;

using Dictionary = std::unordered_map< wxString, Journal::Dispatcher >;
Dictionary &sDictionary()
{
   static Dictionary theDictionary;
   return theDictionary;
}

inline void NextIn()
{
   if ( !sFileIn.Eof() ) {
      sLine = sFileIn.GetNextLine();
      ++sLineNumber;
   }
}

wxArrayStringEx PeekTokens()
{
   wxArrayStringEx tokens;
   if ( Journal::IsReplaying() )
      for ( ; !sFileIn.Eof(); NextIn() ) {
         if ( sLine.StartsWith( CommentCharacter ) )
            continue;
         
         tokens = wxSplit( sLine, SeparatorCharacter );
         if ( tokens.empty() )
            // Ignore blank lines
            continue;
         
         break;
      }
   return tokens;
}

constexpr auto VersionToken = wxT("Version");

// Numbers identifying the journal format version
int journalVersionNumbers[] = {
   1
};

wxString VersionString()
{
   wxString result;
   for ( auto number : journalVersionNumbers ) {
      auto str = wxString::Format( "%d", number );
      result += ( result.empty() ? str : ( '.' + str ) );
   }
   return result;
}

bool VersionCheck( const wxString &value )
{
   auto strings = wxSplit( value, '.' );
   std::vector<int> numbers;
   for ( auto &string : strings ) {
      long value;
      if ( !string.ToCLong( &value ) )
         return false;
      numbers.push_back( value );
   }
   // For now, require the exactly same journal version in the input, as
   // in this build.  In future, there may be some ability to read recent
   // journal versions.
   return !std::lexicographical_compare(
      std::begin( journalVersionNumbers ), std::end( journalVersionNumbers ),
      numbers.begin(), numbers.end() );
}

}

namespace Journal {

SyncException::SyncException()
{
   // If the exception is ever constructed, cause nonzero program exit code
   sError = true;
}

SyncException::~SyncException() {}

void SyncException::DelayedHandlerAction()
{
   // Simulate the application Exit menu item
   wxCommandEvent evt{ wxEVT_MENU, wxID_EXIT };
   wxTheApp->AddPendingEvent( evt );
}

bool RecordEnabled()
{
   static bool initialized = false;
   if ( ! initialized ) {
      sRecordEnabled = gPrefs->ReadBool( EnabledKey, false );
      initialized = true;
   }
   return sRecordEnabled;
}

bool ToggleRecordEnabled()
{
   gPrefs->Write( EnabledKey, ( sRecordEnabled = !sRecordEnabled ) );
   gPrefs->Flush();
   return sRecordEnabled;
}

bool IsRecording()
{
  return sFileOut.IsOpened();
}

bool IsReplaying()
{
  return sFileIn.IsOpened();
}

void SetInputFileName(const wxString &path)
{
   sFileNameIn = path;
}

bool Begin( const FilePath &dataDir )
{
   if ( !sError && !sFileNameIn.empty() ) {
      wxFileName fName{ sFileNameIn };
      fName.MakeAbsolute( dataDir );
      const auto path = fName.GetFullPath();
      sFileIn.Open( path );
      if ( !sFileIn.IsOpened() )
         sError = true;
      else {
         sLine = sFileIn.GetFirstLine();
         sLineNumber = 0;
         
         auto tokens = PeekTokens();
         NextIn();
         sError = !(
            tokens.size() == 2 &&
            tokens[0] == VersionToken &&
            VersionCheck( tokens[1] )
         );
      }
   }

   if ( !sError && RecordEnabled() ) {
      wxFileName fName{ dataDir, "journal", "txt" };
      const auto path = fName.GetFullPath();
      sFileOut.Open( path );
      if ( sFileOut.IsOpened() )
         sFileOut.Clear();
      else  {
         sFileOut.Create();
         sFileOut.Open( path );
      }
      if ( !sFileOut.IsOpened() )
         sError = true;
      else {
         // Generate a header
         Comment( wxString::Format(
            wxT("Journal recorded by %s on %s")
               , wxGetUserName()
               , wxDateTime::Now().Format()
         ) );
         Output({ VersionToken, VersionString() });
      }
   }

   return !sError;
}

wxArrayStringEx GetTokens()
{
   auto result = PeekTokens();
   if ( !result.empty() ) {
      NextIn();
      return result;
   }
   throw SyncException{};
}

RegisteredCommand::RegisteredCommand(
   const wxString &name, Dispatcher dispatcher )
{
   if ( !sDictionary().insert( { name, dispatcher } ).second ) {
      wxLogDebug( wxString::Format (
         wxT("Duplicated registration of Journal command name %s"),
         name
      ) );
      // Cause failure of startup of journalling and graceful exit
      sError = true;
   }
}

bool Dispatch()
{
   if ( sError )
      // Don't repeatedly indicate error
      // Do nothing
      return false;

   if ( !IsReplaying() )
      return false;

   // This will throw if no lines remain.  A proper journal should exit the
   // program before that happens.
   auto words = GetTokens();

   // Lookup dispatch function by the first field of the line
   auto &table = sDictionary();
   auto &name = words[0];
   auto iter = table.find( name );
   if ( iter == table.end() )
      throw SyncException{};

   // Pass all the fields including the command name to the function
   if ( !iter->second( words ) )
      throw SyncException{};

   return true;
}

void Output( const wxString &string )
{
   if ( IsRecording() )
      sFileOut.AddLine( string );
}

void Output( const wxArrayString &strings )
{
   if ( IsRecording() )
      Output( ::wxJoin( strings, SeparatorCharacter ) );
}

void Output( std::initializer_list< const wxString > strings )
{
   if ( IsRecording() ) {
      wxArrayString arr;
      for ( auto &string : strings )
         arr.push_back( string );
      Output( arr );
   }
}

void Comment( const wxString &string )
{
   if ( IsRecording() )
      sFileOut.AddLine( CommentCharacter + string );
}

void CoverageComment( const char *file, int line )
{
   wxString path{ file };
   wxString src{ "/src/" };
   auto pos = path.Find( src );
   if ( pos != wxString::npos )
      std::cerr << wxString::Format( "Covered %s %d\n",
         path.substr( pos + src.length() ), line );
}

void Sync( const wxString &string )
{
   if ( IsRecording() || IsReplaying() ) {
      if ( IsRecording() )
         sFileOut.AddLine( string );
      if ( IsReplaying() ) {
         if ( sFileIn.Eof() || sLine != string )
            throw SyncException{};
         NextIn();
      }
   }
}

void Sync( const wxArrayString &strings )
{
   if ( IsRecording() || IsReplaying() ) {
      auto string = ::wxJoin( strings, SeparatorCharacter );
      Sync( string );
   }
}

void Sync( std::initializer_list< const wxString > strings )
{
   if ( IsRecording() || IsReplaying() ) {
      wxArrayString arr;
      for ( auto &string : strings )
         arr.push_back( string );
      Sync( arr );
   }
}

int GetExitCode()
{
   // Unconsumed commands remaining in the input file is also an error condition.
   if( !sError && !PeekTokens().empty() ) {
      NextIn();
      sError = true;
   }
   if ( sError ) {
      // Return nonzero
      // Returning the (1-based) line number at which the script failed is a
      // simple way to communicate that information to the test driver script.
      return sLineNumber ? sLineNumber : -1;
   }

   // Return zero to mean all is well, the convention for command-line tools
   return 0;
}

}
