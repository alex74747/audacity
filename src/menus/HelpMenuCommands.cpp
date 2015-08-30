#include "../Audacity.h"
#include "HelpMenuCommands.h"

#include <wx/msgdlg.h>

#include "../AudioIO.h"
#include "../Benchmark.h"
#include "../FileDialog.h"
#include "../Project.h"
#include "../Screenshot.h"
#include "../ShuttleGUI.h"
#include "../commands/CommandManager.h"
#include "../widgets/HelpSystem.h"
#include "../widgets/LinkingHtmlWindow.h"

#define FN(X) new ObjectCommandFunctor<HelpMenuCommands>(this, &HelpMenuCommands:: X )

HelpMenuCommands::HelpMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void HelpMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddItem(wxT("QuickHelp"), _("&Quick Help"), FN(OnQuickHelp));
   c->AddItem(wxT("Manual"), _("&Manual"), FN(OnManual));

   c->AddSeparator();

   c->AddItem(wxT("Screenshot"), _("&Screenshot Tools..."), FN(OnScreenshot));
#if IS_ALPHA
   // TODO: What should we do here?  Make benchmark a plug-in?
   // Easy enough to do.  We'd call it mod-self-test.
   c->AddItem(wxT("Benchmark"), _("&Run Benchmark..."), FN(OnBenchmark));
#endif

   c->AddSeparator();

   c->AddItem(wxT("Updates"), _("&Check for Updates..."), FN(OnCheckForUpdates));
   c->AddItem(wxT("DeviceInfo"), _("Au&dio Device Info..."), FN(OnAudioDeviceInfo),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);
}

void HelpMenuCommands::OnQuickHelp()
{
   HelpSystem::ShowHelpDialog(
      mProject,
      wxT("Quick_Help"));
}

void HelpMenuCommands::OnManual()
{
   HelpSystem::ShowHelpDialog(
      mProject,
      wxT("Main_Page"));
}

void HelpMenuCommands::OnScreenshot()
{
   ::OpenScreenshotTools();
}

void HelpMenuCommands::OnBenchmark()
{
   ::RunBenchmark(mProject);
}

void HelpMenuCommands::OnCheckForUpdates()
{
   ::OpenInDefaultBrowser(wxString(wxT("http://audacityteam.org/download/?from_ver=")) + AUDACITY_VERSION_STRING);
}

void HelpMenuCommands::OnAudioDeviceInfo()
{
   wxString info = gAudioIO->GetDeviceInfo();

   wxDialog dlg(mProject, wxID_ANY, wxString(_("Audio Device Info")));
   dlg.SetName(dlg.GetTitle());
   ShuttleGui S(&dlg, eIsCreating);

   wxTextCtrl *text;
   S.StartVerticalLay();
   {
      S.SetStyle(wxTE_MULTILINE | wxTE_READONLY);
      text = S.Id(wxID_STATIC).AddTextWindow(info);
      S.AddStandardButtons(eOkButton | eCancelButton);
   }
   S.EndVerticalLay();

   dlg.FindWindowById(wxID_OK)->SetLabel(_("&Save"));
   dlg.SetSize(350, 450);

   if (dlg.ShowModal() == wxID_OK)
   {
      wxString fName = FileSelector(_("Save Device Info"),
         wxEmptyString,
         wxT("deviceinfo.txt"),
         wxT("txt"),
         wxT("*.txt"),
         wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
         mProject);
      if (!fName.IsEmpty())
      {
         if (!text->SaveFile(fName))
         {
            wxMessageBox(_("Unable to save device info"), _("Save Device Info"));
         }
      }
   }
}
