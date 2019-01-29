//
//  wxPanelWrapper.h
//  Audacity
//
//  Created by Paul Licameli on 6/25/16.
//
//

#ifndef __AUDACITY_WXPANEL_WRAPPER__
#define __AUDACITY_WXPANEL_WRAPPER__

#include <type_traits>
#include <memory>
#include <wx/panel.h> // to inherit
#include <wx/dialog.h> // to inherit

#include "Internat.h"

#include "Identifier.h"

namespace DialogHooks {

AUDACITY_DLL_API void wxTabTraversalWrapperCharHook(wxKeyEvent &event);

// Subclass responsibilities for modal dialogs that record and play back
// with parameters
struct CallbacksBase
{
   virtual ~CallbacksBase();

   // This is supplied by the template below but may be further overriden to
   // add behavior
   virtual int DoShowModal( wxDialog &self ) = 0;
};

template< typename Ancestor > struct Callbacks : CallbacksBase
{
   // Supply DoShowModal, but it may be further overridden
   int DoShowModal( wxDialog &dialog ) override
   {
      // The qualified call makes sure to avoid
      // JournallingDialog< Ancestor >::ShowModal which would cause infinite
      // recursion
      return static_cast< Ancestor& >( dialog ).Ancestor::ShowModal();
   }
};

int ShowModal( wxDialog &dialog, CallbacksBase &callbacks );

}

// Decorator class template adds certain special keystroke handling to its base
template <typename Base>
class AUDACITY_DLL_API wxTabTraversalWrapper : public Base
{
public:
   template <typename... Args>
   wxTabTraversalWrapper(Args&&... args)
   : Base( std::forward<Args>(args)... )
   {
      this->Bind(wxEVT_CHAR_HOOK, DialogHooks::wxTabTraversalWrapperCharHook);
   }

   wxTabTraversalWrapper(const wxTabTraversalWrapper&) = delete;
   wxTabTraversalWrapper& operator=(const wxTabTraversalWrapper&) = delete;
   wxTabTraversalWrapper(wxTabTraversalWrapper&&) = delete;
   wxTabTraversalWrapper& operator=(wxTabTraversalWrapper&&) = delete;

};

// Add keystroke handling to wxPanel, and also make a default window name that
// is localized.
class AUDACITY_DLL_API wxPanelWrapper : public wxTabTraversalWrapper<wxPanel>
{
   using Base = wxTabTraversalWrapper<wxPanel>;
public:
   // Constructors
   wxPanelWrapper() {}

   wxPanelWrapper(
         wxWindow *parent,
         wxWindowID winid = wxID_ANY,
         const wxPoint& pos = wxDefaultPosition,
         const wxSize& size = wxDefaultSize,
         long style = wxTAB_TRAVERSAL | wxNO_BORDER,
         // Important:  default window name localizes!
         const wxString& name = _("Panel"))
   : Base( parent, winid, pos, size, style, name )
   {}

    // Pseudo ctor
    bool Create(
         wxWindow *parent,
         wxWindowID winid = wxID_ANY,
         const wxPoint& pos = wxDefaultPosition,
         const wxSize& size = wxDefaultSize,
         long style = wxTAB_TRAVERSAL | wxNO_BORDER,
         // Important:  default window name localizes!
         const TranslatableString& name = XO("Panel"))
   {
      return Base::Create(
         parent, winid, pos, size, style, name.Translation()
      );
   }
   // overload and hide the inherited functions that take naked wxString:
   void SetLabel(const TranslatableString & label);
   void SetName(const TranslatableString & name);
   void SetToolTip(const TranslatableString &toolTip);
   // Set the name to equal the label:
   void SetName();
};

// Add keystroke handling to some ancestor class (that is wxDialog or derived
// therefrom), and also insert code related to recording and playback of
// journals.
template< typename Ancestor = wxDialog >
class AUDACITY_DLL_API JournallingDialog
   : public wxTabTraversalWrapper< Ancestor >
   , public DialogHooks::Callbacks< Ancestor >
{
   using Base = wxTabTraversalWrapper< Ancestor >;
public:
   template< typename ...Args >
   JournallingDialog( Args &&...args )
      : Base( std::forward< Args >( args )... )
   { }

   JournallingDialog( const JournallingDialog& ) = delete;
   JournallingDialog &operator =( const JournallingDialog& ) = delete;

   ~JournallingDialog()
   { }

   // Further derived classes should not override ShowModal, but instead
   // override Callbacks::DoShowModal

   int ShowModal() final override
   {
      static_assert(std::is_base_of< wxDialog, Ancestor >::value,
         "template parameter must be wxDialog or a subclass");
      return DialogHooks::ShowModal( *this, *this );
   }
};

// Add keystroke handling and journalling to wxDialog, and also make a default
// window name that is localized.
class AUDACITY_DLL_API wxDialogWrapper
   : public JournallingDialog<>
{
   using Base = JournallingDialog<>;
public:
   // Constructors
   wxDialogWrapper() {}
   
   wxDialogWrapper( const wxDialogWrapper& ) = delete;
   wxDialogWrapper &operator =( const wxDialogWrapper& ) = delete;

   // Constructor with no modal flag - the new convention.
   wxDialogWrapper(
      wxWindow *parent, wxWindowID id,
      const TranslatableString& title,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& size = wxDefaultSize,
      long style = wxDEFAULT_DIALOG_STYLE,
      // Important:  default window name localizes!
      const TranslatableString& name = XO("Dialog"))
   : Base( parent, id, title.Translation(), pos, size, style, name.Translation() )
   {}

   // Pseudo ctor
   bool Create(
      wxWindow *parent, wxWindowID id,
      const TranslatableString& title,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& size = wxDefaultSize,
      long style = wxDEFAULT_DIALOG_STYLE,
      // Important:  default window name localizes!
      const TranslatableString& name = XO("Dialog"))
   {
      return Base::Create(
         parent, id, title.Translation(), pos, size, style, name.Translation()
      );
   }

   // overload and hide the inherited functions that take naked wxString:
   void SetTitle(const TranslatableString & title);
   void SetLabel(const TranslatableString & title);
   void SetName(const TranslatableString & title);
   // Set the name to equal the title:
   void SetName();
};

#include <wx/dirdlg.h> // to inherit

// Add keystroke handling and journalling to wxDirDialog, and also make a default
// window name that is localized.
class AUDACITY_DLL_API wxDirDialogWrapper
   : public JournallingDialog<wxDirDialog>
{
   using Base = JournallingDialog<wxDirDialog>;
public:
   // Constructor with no modal flag - the new convention.
   wxDirDialogWrapper(
      wxWindow *parent,
      const TranslatableString& message = XO("Select a directory"),
      const wxString& defaultPath = {},
      long style = wxDD_DEFAULT_STYLE,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& size = wxDefaultSize,
      // Important:  default window name localizes!
      const TranslatableString& name = XO("Directory Dialog"))
   : Base(
      parent, message.Translation(), defaultPath, style, pos, size,
      name.Translation() )
   {}

   wxDirDialogWrapper( const wxDirDialogWrapper & ) = delete;
   wxDirDialogWrapper &operator =( const wxDirDialogWrapper & ) = delete;

   // Pseudo ctor
   void Create(
      wxWindow *parent,
      const TranslatableString& message = XO("Select a directory"),
      const wxString& defaultPath = {},
      long style = wxDD_DEFAULT_STYLE,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& size = wxDefaultSize,
      // Important:  default window name localizes!
      const TranslatableString& name = XO("Directory Dialog"))
   {
      Base::Create(
         parent, message.Translation(), defaultPath, style, pos, size,
         name.Translation() );
   }
};

#include "FileDialog/FileDialog.h"
#include "../FileNames.h" // for FileTypes

// Add keystroke handling and journalling to FileDialog, and also make a default
// window name that is localized.
class AUDACITY_DLL_API FileDialogWrapper
   : public JournallingDialog<FileDialog>
{
   using Base = JournallingDialog<FileDialog>;
public:
   FileDialogWrapper() {}

   // Constructor with no modal flag - the new convention.
   FileDialogWrapper(
      wxWindow *parent,
      const TranslatableString& message,
      const FilePath& defaultDir,
      const FilePath& defaultFile,
      const FileNames::FileTypes& fileTypes,
      long style = wxFD_DEFAULT_STYLE,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& sz = wxDefaultSize,
      // Important:  default window name localizes!
      const TranslatableString& name = XO("File Dialog"))
   : Base(
      parent, message.Translation(), defaultDir, defaultFile,
      FileNames::FormatWildcard( fileTypes ),
      style,
      pos, sz, name.Translation() )
   {}

   FileDialogWrapper( const FileDialogWrapper & ) = delete;
   FileDialogWrapper &operator =( const FileDialogWrapper & ) = delete;

   // Pseudo ctor
   void Create(
      wxWindow *parent,
      const TranslatableString& message,
      const FilePath& defaultDir,
      const FilePath& defaultFile,
      const FileNames::FileTypes& fileTypes,
      long style = wxFD_DEFAULT_STYLE,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& sz = wxDefaultSize,
      // Important:  default window name localizes!
      const TranslatableString& name = XO("File Dialog"))
   {
      Base::Create(
         parent, message.Translation(), defaultDir, defaultFile,
         FileNames::FormatWildcard( fileTypes ),
         style, pos, sz, name.Translation()
      );
   }
};

#include <wx/msgdlg.h>

/**************************************************************************//**

\brief Wrap wxMessageDialog so that caption IS translatable.
********************************************************************************/
class AudacityMessageDialog
   : public JournallingDialog< wxMessageDialog >
{
   using Base = JournallingDialog< wxMessageDialog >;
public:
    AudacityMessageDialog(
         wxWindow *parent,
         const TranslatableString &message,
         const TranslatableString &caption, // don't use = wxMessageBoxCaptionStr,
         long style = wxOK|wxCENTRE,
         const wxPoint& pos = wxDefaultPosition)
   : Base(
      parent, message.Translation(), caption.Translation(), style, pos )
   {
      SetName(_("Message"));
   }

};

#endif
