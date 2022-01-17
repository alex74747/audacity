/**********************************************************************

Audacity: A Digital Audio Editor

PopupMenuTable.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/


#include "PopupMenuTable.h"
#include "BasicMenu.h"
#include "wxWidgetsWindowPlacement.h"
#include "MenuHandle.h"

#include <wx/menu.h>
#include <wx/window.h>

PopupMenuTableEntry::~PopupMenuTableEntry()
{}

PopupSubMenu::~PopupSubMenu()
{}

PopupSubMenu::PopupSubMenu( const Identifier &stringId,
   const TranslatableString &caption_, PopupMenuTable &table_ )
   : ConcreteGroupItem< false >{ stringId }
   , WholeMenu{ caption_.empty() }
   , caption{ caption_ }
   , table{ table_ }
{
}

PopupMenu::~PopupMenu() = default;

namespace {
struct PopupMenuImpl : PopupMenu, wxMenu
{
   PopupMenuImpl(void *pUserData_)
      : pUserData{ pUserData_ } {}

   ~PopupMenuImpl() override;

   void Popup( wxWindow &window, const wxPoint &pos ) override;

   void Extend(PopupMenuTable *pTable);

   void *pUserData;
   Widgets::MenuHandle mMenu;
};

class PopupMenuBuilder : public PopupMenuVisitor {
public:
   explicit
   PopupMenuBuilder( PopupMenuTable &table, PopupMenuImpl &menu, void *pUserData )
      : PopupMenuVisitor{ table }
      , mMenu{ &menu }
      , mRoot{ mMenu }
      , mpUserData{ pUserData }
   {}

   void DoBeginGroup( Registry::GroupItem &item, const Path &path ) override;
   void DoEndGroup( Registry::GroupItem &item, const Path &path ) override;
   void DoVisit( Registry::SingleItem &item, const Path &path ) override;
   void DoSeparator() override;

   std::vector< std::unique_ptr<PopupMenuImpl> > mMenus;
   PopupMenuImpl *mMenu, *mRoot;
   void *const mpUserData;
};

void PopupMenuBuilder::DoBeginGroup( Registry::GroupItem &item, const Path &path )
{
   if ( auto pItem = dynamic_cast<PopupSubMenu*>(&item) ) {
      if ( !pItem->caption.empty() ) {
         auto newMenu =
            std::make_unique<PopupMenuImpl>( mMenu->pUserData );
         mMenu = newMenu.get();
         mMenus.push_back( std::move( newMenu ) );
      }
   }
}

void PopupMenuBuilder::DoEndGroup( Registry::GroupItem &item, const Path &path )
{
   if ( auto pItem = dynamic_cast<PopupSubMenu*>(&item) ) {
      if ( !pItem->caption.empty() ) {
         auto subMenu = std::move( mMenus.back() );
         mMenus.pop_back();
         mMenu = mMenus.empty() ? mRoot : mMenus.back().get();
         mMenu->mMenu.AppendSubMenu(
            std::move( subMenu->mMenu ), pItem->caption );
      }
   }
}

void PopupMenuBuilder::DoVisit( Registry::SingleItem &item, const Path &path )
{
   auto pEntry = static_cast<PopupMenuTableEntry*>( &item );
   const auto caption = pEntry->caption.FullLabel();
   switch (pEntry->type) {
      case PopupMenuTable::Entry::Item:
      {
         mMenu->mMenu.Append(caption, {}, {}, pEntry->id);
         break;
      }
      case PopupMenuTable::Entry::RadioItem:
      {
         mMenu->mMenu.AppendRadioItem(caption, {}, {}, pEntry->id);
         break;
      }
      case PopupMenuTable::Entry::CheckItem:
      {
         mMenu->mMenu.AppendCheckItem(caption, {}, {}, pEntry->id);
         break;
      }
      default:
         wxASSERT( false );
         break;
   }

   // This call necessary for externally registered items, else harmlessly
   // redundant
   pEntry->handler.InitUserData( mpUserData );

   if ( pEntry->stateFn )
      mMenu->mMenu.SetState( pEntry->id, pEntry->stateFn() );

   mMenu->Bind(
      wxEVT_COMMAND_MENU_SELECTED, pEntry->func, &pEntry->handler, pEntry->id);
}

void PopupMenuBuilder::DoSeparator()
{
   mMenu->mMenu.AppendSeparator();
}

PopupMenuImpl::~PopupMenuImpl()
{
   // Event connections between the parent window and the singleton table
   // object must be broken when this menu is destroyed.
   Disconnect();
}

void PopupMenuImpl::Popup( wxWindow &window, const wxPoint &pos )
{
   BasicMenu::Handle{ &this->mMenu }.Popup(
      wxWidgetsWindowPlacement{ &window }, { pos.x, pos.y }
   );
}
}

void PopupMenuTable::ExtendMenu( PopupMenu &menu, PopupMenuTable &table )
{
   auto &theMenu = dynamic_cast<PopupMenuImpl&>(menu);

   PopupMenuBuilder visitor{ table, theMenu, theMenu.pUserData };
   Registry::Visit(
      visitor, table.Get( theMenu.pUserData ).get(), table.GetRegistry() );
}

void PopupMenuTable::RegisterItem(
   const Registry::Placement &placement, Registry::BaseItemPtr pItem )
{
   Registry::RegisterItem( *mRegistry, placement, std::move( pItem ) );
}

void PopupMenuTable::Append( Registry::BaseItemPtr pItem )
{
   mStack.back()->items.push_back( std::move( pItem ) );
}

void PopupMenuTable::Append(
   const Identifier &stringId, PopupMenuTableEntry::Type type, int id,
   const Widgets::MenuItemLabel &string, wxCommandEventFunction memFn,
   const PopupMenuTableEntry::StateFunction &stateFn )
{
   Append( std::make_unique<Entry>(
      stringId, type, id, string, memFn, *this, stateFn ) );
}

void PopupMenuTable::BeginSection( const Identifier &name )
{
   auto uSection = std::make_unique< PopupMenuSection >( name );
   auto section = uSection.get();
   mStack.back()->items.push_back( std::move( uSection ) );
   mStack.push_back( section );
}

void PopupMenuTable::EndSection()
{
   mStack.pop_back();
}

// static
std::unique_ptr< PopupMenu > PopupMenuTable::BuildMenu(
   PopupMenuTable *pTable, void *pUserData )
{
   // Rebuild as needed each time.  That makes it safe in case of language change.
   auto theMenu = std::make_unique<PopupMenuImpl>( pUserData );
   ExtendMenu( *theMenu, *pTable );
   return std::move( theMenu );
}

