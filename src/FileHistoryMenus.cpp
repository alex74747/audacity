/**********************************************************************

  Audacity: A Digital Audio Editor

  @file FileHistoryMenus.cpp

  Paul Licameli split from FileHistory.cpp

**********************************************************************/


#include "FileHistoryMenus.h"

#include <wx/menu.h>

#include "Internat.h"

void FileHistoryMenus::UseMenu(wxMenu *menu)
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
   for (auto pMenu : mMenus)
      if (pMenu)
         NotifyMenu(pMenu);
}

void FileHistoryMenus::NotifyMenu(wxMenu *menu)
{
   wxMenuItemList items = menu->GetMenuItems();
   for (auto end = items.end(), iter = items.begin(); iter != end;)
      menu->Destroy(*iter++);

   const auto &history = FileHistory::Global();
   int mIDBase = ID_RECENT_CLEAR;
   int i = 0;
   for (auto item : history) {
      item.Replace( "&", "&&" );
      menu->Append(mIDBase + 1 + i++, item);
   }

   if (history.size() > 0) {
      menu->AppendSeparator();
   }
   menu->Append(mIDBase, _("&Clear"));
   menu->Enable(mIDBase, history.size() > 0);
}

void FileHistoryMenus::Compress()
{
   // Clear up expired weak pointers
   auto end = mMenus.end();
   mMenus.erase(
     std::remove_if( mMenus.begin(), end,
        [](wxWeakRef<wxMenu> &pMenu){ return !pMenu; } ),
     end
   );
}

