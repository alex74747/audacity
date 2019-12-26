/**********************************************************************

  Audacity: A Digital Audio Editor

  wxMenuWrapper.h
  @brief Wrap wxMenu in alternative interface that requires TranslatableString arguments

  Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_WX_MENU_WRAPPER__
#define __AUDACITY_WX_MENU_WRAPPER__

#include <wx/menu.h>
/**************************************************************************//**

\brief Wrap wxMenu so that you must supply translated strings
********************************************************************************/
class wxMenuWrapper : public wxMenu
{
public:
   wxMenu *get() { return this; }

   wxMenuWrapper() = default;

   wxMenuItem* Append(int itemid,
      const TranslatableString& text = {},
      const TranslatableString& help = {},
      wxItemKind kind = wxITEM_NORMAL)
   {
      return wxMenu::Append(
         itemid, text.Translation(), help.Translation(), kind );
   }

   wxMenuItem* AppendSubMenu(wxMenuWrapper *submenu,
      const TranslatableString& text,
      const TranslatableString& help = {})
   {
      return wxMenu::AppendSubMenu(
         submenu, text.Translation(), help.Translation() );
   }

    wxMenuItem* Append(int itemid,
                       const TranslatableString& text,
                       wxMenuWrapper *submenu,
                       const TranslatableString& help = {})
   {
      return wxMenu::Append(itemid, text.Translation(), submenu, help.Translation());
   }

   wxMenuItem* AppendRadioItem(int itemid,
      const TranslatableString& text,
      const TranslatableString& help = {})
   {
      return wxMenu::AppendRadioItem(
         itemid, text.Translation(), help.Translation());
   }

   wxMenuItem* AppendCheckItem(int itemid,
      const TranslatableString& text,
      const TranslatableString& help = {})
   {
      return wxMenu::AppendCheckItem(
         itemid, text.Translation(), help.Translation());
   }

   void SetLabel(int itemid, const TranslatableString& label)
   {
      wxMenu::SetLabel( itemid, label.Translation() );
   }

   TranslatableString GetLabel(int itemid) const = delete;
   /*
   {
      return Verbatim( wxMenu::GetLabel( itemid ) );
   }
   */

   wxMenuWrapper *GetParent() const
   {
      return dynamic_cast<wxMenuWrapper*>( wxMenu::GetParent() );
   }
   
   void SetHelpString(int itemid, const TranslatableString& helpString)
   {
      wxMenu::SetHelpString( itemid, helpString.Translation() );
   }

   using wxMenu::Enable;
   using wxMenu::IsEnabled;
   using wxMenu::Bind;
   using wxMenu::AppendSeparator;
   using wxMenu::Check;
   using wxMenu::IsChecked;
   using wxMenu::GetMenuItems;
   using wxMenu::FindItem;
   using wxMenu::Destroy;
};

#endif
