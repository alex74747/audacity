/**********************************************************************

  Audacity: A Digital Audio Editor

  @file FileHistoryMenus.cpp

  Paul Licameli split from FileHistory.cpp

**********************************************************************/


#include "widgets/BasicMenu.h"
#include "FileHistoryMenus.h"
#include "ProjectManager.h" // Cycle

#include <wx/menu.h>

#include "Internat.h"

void FileHistoryMenus::UseMenu(BasicMenu::Handle menu)
{
   Compress();

   auto end = mMenus.end();
   auto iter = std::find(mMenus.begin(), end, menu);
   auto found = (iter != end);

   if (!found)
      mMenus.push_back(menu);
   else {
      wxASSERT(false);
   }

   NotifyMenu( menu );
}

FileHistoryMenus::FileHistoryMenus()
{
   mSubscription = FileHistory::Global()
      .Subscribe(*this, &FileHistoryMenus::OnChangedHistory);
}

FileHistoryMenus &FileHistoryMenus::Instance()
{
   static FileHistoryMenus instance;
   return instance;
}

void FileHistoryMenus::OnChangedHistory(Observer::Message)
{
   Compress();
   for (auto &pMenu : mMenus)
      if (pMenu)
         NotifyMenu(pMenu);
}

void FileHistoryMenus::NotifyMenu(BasicMenu::Handle menu)
{
   menu.Clear();

   const auto &history = FileHistory::Global();
   int ii = 0;
   for (auto item : history) {
      item.Replace( "&", "&&" );
      menu.Append( Verbatim( item ), [ii]{ ProjectManager::OnMRUFile(ii); } );
   }

   if (history.size() > 0)
      menu.AppendSeparator();

   menu.Append( XXO("&Clear"),
      &ProjectManager::OnMRUClear, history.size() > 0 );
}

void FileHistoryMenus::Compress()
{
   // Clear up expired weak pointers
   auto end = mMenus.end();
   mMenus.erase(
     std::remove_if( mMenus.begin(), end,
        [](auto &pMenu){ return !pMenu; } ),
     end
   );
}
