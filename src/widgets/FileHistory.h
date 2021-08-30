/**********************************************************************

  Audacity: A Digital Audio Editor

  FileHistory.h

  Leland Lucius

**********************************************************************/

#ifndef __AUDACITY_WIDGETS_FILEHISTORY__
#define __AUDACITY_WIDGETS_FILEHISTORY__

#include <vector>
#include <algorithm>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/weakref.h> // member variable

#include "Identifier.h"
#include "wxArrayStringEx.h"

class wxConfigBase;
class wxMenu;

//! Event emitted by the global FileHistory when its contents change
DECLARE_EXPORTED_EVENT_TYPE(AUDACITY_DLL_API, EVT_FILE_HISTORY_CHANGE, -1);

class AUDACITY_DLL_API FileHistory
   : public wxEvtHandler
{
 public:
   enum { MAX_FILES = 12 };
   FileHistory(size_t maxfiles = MAX_FILES);
   virtual ~FileHistory();
   FileHistory( const FileHistory& ) = delete;
   FileHistory &operator =( const FileHistory & ) = delete;

   static FileHistory &Global();

   void Append( const FilePath &file )
   { AddFileToHistory( file, true ); }
   void Remove( size_t i );
   void Clear();

   void Load(wxConfigBase& config, const wxString & group = wxEmptyString);
   void Save(wxConfigBase& config);

   // stl-style accessors
   using const_iterator = FilePaths::const_iterator;
   const_iterator begin() const { return mHistory.begin(); }
   const_iterator end() const { return mHistory.end(); }
   const FilePath &operator[] ( size_t ii ) const { return mHistory[ ii ]; }
   bool empty() const { return mHistory.empty(); }
   size_t size() const { return mHistory.size(); }

 private:
   void AddFileToHistory(const FilePath & file, bool update);
   void NotifyMenus();

   size_t mMaxFiles;

   FilePaths mHistory;

   wxString mGroup;
};

class FileHistoryMenus : public wxEvtHandler {
private:
   FileHistoryMenus();

public:
   static FileHistoryMenus &Instance();

   // These constants define the range of IDs reserved by the global file history
   enum {
      ID_RECENT_CLEAR = 6100,
      ID_RECENT_FIRST = 6101,
      ID_RECENT_LAST  = ID_RECENT_FIRST + FileHistory::MAX_FILES - 1,
   };

   // Make the menu reflect the contents of the global FileHistory,
   // now and also whenever the history changes.
   void UseMenu(wxMenu *menu);
   
private:
   void OnChangedHistory(wxEvent &evt);
   void NotifyMenu(wxMenu *menu);
   std::vector< wxWeakRef< wxMenu > > mMenus;

   void Compress();
};

#endif
