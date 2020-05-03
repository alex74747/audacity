/**********************************************************************

  Audacity: A Digital Audio Editor

  FileHistory.cpp

  Leland Lucius

*******************************************************************//**

\class FileHistory
\brief Similar to wxFileHistory, but customized to our needs.

*//*******************************************************************/


#include "FileHistory.h"

#include <wx/defs.h>

#include "Internat.h"
#include "Prefs.h"

#include <mutex>

DEFINE_EVENT_TYPE(EVT_FILE_HISTORY_CHANGE);

namespace {
struct MyEvent : wxEvent
{
public:
   explicit MyEvent() : wxEvent{ 0, EVT_FILE_HISTORY_CHANGE } {}
   virtual wxEvent *Clone() const override { return new MyEvent{}; }
};
}

FileHistory::FileHistory(size_t maxfiles)
{
   mMaxFiles = maxfiles;
}

FileHistory::~FileHistory()
{
}

FileHistory &FileHistory::Global()
{
   // TODO - read the number of files to store in history from preferences
   static FileHistory history;
   static std::once_flag flag;
   std::call_once( flag, [&]{
      history.Load(*gPrefs, L"RecentFiles");
   });

   return history;
}

auto FileHistory::begin() const -> const_iterator
{
   return mHistory.begin();
}

auto FileHistory::end() const -> const_iterator
{
   return mHistory.end();
}

const FilePath &FileHistory::operator[] ( size_t ii ) const
{
   return mHistory[ ii ];
}

bool FileHistory::empty() const
{
   return mHistory.empty();
}

size_t FileHistory::size() const
{
   return mHistory.size();
}

// File history management
void FileHistory::AddFileToHistory(const FilePath & file, bool update)
{
   // Needed to transition from wxFileHistory to FileHistory since there
   // can be empty history "slots".
   if (file.empty()) {
      return;
   }

#if defined(__WXMSW__)
   int i = mHistory.Index(file, false);
#else
   int i = mHistory.Index(file, true);
#endif

   if (i != wxNOT_FOUND) {
      mHistory.erase( mHistory.begin() + i );
   }

   if (mMaxFiles > 0 && mMaxFiles == mHistory.size()) {
      mHistory.erase( mHistory.end() - 1 );
   }

   mHistory.insert(mHistory.begin(), file);

   if (update)
      NotifyMenus();
}

void FileHistory::Remove( size_t i )
{
   wxASSERT(i < mHistory.size());

   if (i < mHistory.size()) {
      mHistory.erase( mHistory.begin() + i );

      NotifyMenus();
   }
}

void FileHistory::Clear()
{
   mHistory.clear();

   NotifyMenus();
}

void FileHistory::Load(wxConfigBase & config, const wxString & group)
{
   mHistory.clear();
   mGroup = group.empty()
      ? wxString{ "RecentFiles" }
      : group;

   config.SetPath(mGroup);

   wxString file;
   long ndx;
   bool got = config.GetFirstEntry(file, ndx);
   while (got) {
      AddFileToHistory(config.Read(file), false);
      got = config.GetNextEntry(file, ndx);
   }

   config.SetPath(L"..");

   NotifyMenus();
}

void FileHistory::Save(wxConfigBase & config)
{
   config.SetPath(L"");
   config.DeleteGroup(mGroup);
   config.SetPath(mGroup);

   // Stored in reverse order
   int n = mHistory.size() - 1;
   for (size_t i = 1; i <= mHistory.size(); i++) {
      config.Write(wxString::Format(L"file%02d", (int)i), mHistory[n--]);
   }

   config.SetPath(L"");

   config.Flush();
}

void FileHistory::NotifyMenus()
{
   MyEvent event;
   ProcessEvent(event);
   Save(*gPrefs);
}
