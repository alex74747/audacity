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

#include "BasicMenu.h"
#include "Identifier.h"
#include "wxArrayStringEx.h"

class wxConfigBase;

namespace Widgets {
   class MenuHandle;
}

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
   const_iterator begin() const;
   const_iterator end() const;
   const FilePath &operator[] ( size_t ii ) const;
   bool empty() const;
   size_t size() const;

 private:
   void AddFileToHistory(const FilePath & file, bool update);
   void NotifyMenus();

   size_t mMaxFiles;

   FilePaths mHistory;

   wxString mGroup;
};

#endif
