/**********************************************************************

  Audacity: A Digital Audio Editor

  @file FileHistoryMenus.h

  Paul Licameli split from FileHistory.h

**********************************************************************/

#ifndef __AUDACITY_WIDGETS_FILE_HISTORY_MENUS__
#define __AUDACITY_WIDGETS_FILE_HISTORY_MENUS__

#include "widgets/FileHistory.h"
#include "Observer.h"

// These constants define the range of IDs reserved by the global file history
enum {
   ID_RECENT_CLEAR = 6100,
   ID_RECENT_FIRST = 6101,
};

class FileHistoryMenus {
private:
   FileHistoryMenus();

public:
   static FileHistoryMenus &Instance();

   // Make the menu reflect the contents of the global FileHistory,
   // now and also whenever the history changes.
   void UseMenu(wxMenu *menu);
   
private:
   void OnChangedHistory(Observer::Message);
   void NotifyMenu(wxMenu *menu);
   std::vector< wxWeakRef< wxMenu > > mMenus;
   Observer::Subscription mSubscription;

   void Compress();
} ;

#endif
