#include "../Audacity.h"
#include "EditMenuCommands.h"

#include "../HistoryWindow.h"
#include "../MixerBoard.h"
#include "../Project.h"
#include "../TrackPanel.h"
#include "../UndoManager.h"
#include "../commands/CommandManager.h"

#include <wx/msgdlg.h>

#define FN(X) FNT(EditMenuCommands, this, & EditMenuCommands :: X)

EditMenuCommands::EditMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void EditMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag,
      AudioIONotBusyFlag | TimeSelectedFlag | TracksSelectedFlag);

   c->AddItem(wxT("Undo"), _("&Undo"), FN(OnUndo), wxT("Ctrl+Z"),
      AudioIONotBusyFlag | UndoAvailableFlag,
      AudioIONotBusyFlag | UndoAvailableFlag);
}

void EditMenuCommands::OnUndo()
{
   if (!mProject->GetUndoManager()->UndoAvailable()) {
      wxMessageBox(_("Nothing to undo"));
      return;
   }

   // can't undo while dragging
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   if (trackPanel->IsMouseCaptured()) {
      return;
   }

   const auto &state = mProject->GetUndoManager()->Undo(&mProject->GetViewInfo().selectedRegion);
   mProject->PopState(state);

   trackPanel->SetFocusedTrack(NULL);
   trackPanel->EnsureVisible(trackPanel->GetFirstSelectedTrack());

   mProject->RedrawProject();

   HistoryWindow *const historyWindow = mProject->GetHistoryWindow();
   if (historyWindow)
      historyWindow->UpdateDisplay();

   if (auto mixerBoard = mProject->GetMixerBoard())
      // Mixer board may need to change for selection state and pan/gain
      mixerBoard->Refresh();
   
   mProject->ModifyUndoMenuItems();
}
