/**********************************************************************

  Audacity: A Digital Audio Editor

  BatchCommandDialog.h

  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_MACRO_COMMAND_DIALOG__
#define __AUDACITY_MACRO_COMMAND_DIALOG__

#include <wx/defs.h>

#include "BatchCommands.h"

class wxWindow;
class wxTextCtrl;
class wxListCtrl;
class wxListEvent;
class wxButton;
class AudacityProject;
class ShuttleGui;

class MacroCommandDialog final : public wxDialogWrapper {
 public:
   // constructors and destructors
   MacroCommandDialog(wxWindow *parent, wxWindowID id, AudacityProject &project);
   void SetCommandAndParams(const CommandID &Command, const wxString &Params);
 public:
   CommandID   mSelectedCommand;
   wxString   mSelectedParameters;
 private:
   void Populate();
   void PopulateOrExchange(ShuttleGui &S);
   void OnEditParams();
   void OnUsePreset();
   void OnChoice(wxCommandEvent &event);
   void OnOk();
   void OnCancel();
   void OnHelp();
   void OnItemSelected(wxListEvent &event);
   ManualPageID GetHelpPageName() { return L"Scripting Reference" ; }

   void ValidateChoices();
   void PopulateCommandList();
   //int GetSelectedItem();

   wxListCtrl *mChoices;
   wxTextCtrl * mCommand;
   wxTextCtrl * mParameters;
   wxTextCtrl * mDetails;

   CommandID mInternalCommandName;
   // These two are recomputed when the previous changes:
   PluginID mPluginID{};
   bool mHasPresets = false;

   const MacroCommandsCatalog mCatalog;

   DECLARE_EVENT_TABLE()
};


#endif
