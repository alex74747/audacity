#include "EditMenuCommands.h"

#include "../HistoryWindow.h"
#include "../Project.h"
#include "../TrackPanel.h"
#include "../commands/CommandManager.h"

#include <wx/msgdlg.h>

#define FN(X) new ObjectCommandFunctor<EditMenuCommands>(this, &EditMenuCommands:: X )

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

   // The default shortcut key for Redo is different on different platforms.
   wxString key =
#ifdef __WXMSW__
      wxT("Ctrl+Y");
#else
      wxT("Ctrl+Shift+Z");
#endif

   c->AddItem(wxT("Redo"), _("&Redo"), FN(OnRedo), key,
      AudioIONotBusyFlag | RedoAvailableFlag,
      AudioIONotBusyFlag | RedoAvailableFlag);

   mProject->ModifyUndoMenuItems();
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

   TrackList *const l = mProject->GetUndoManager()->Undo(&mProject->GetViewInfo().selectedRegion);
   mProject->PopState(l);

   trackPanel->SetFocusedTrack(NULL);
   trackPanel->EnsureVisible(trackPanel->GetFirstSelectedTrack());

   mProject->RedrawProject();

   HistoryWindow *const historyWindow = mProject->GetHistoryWindow();
   if (historyWindow)
      historyWindow->UpdateDisplay();

   mProject->ModifyUndoMenuItems();
}

void EditMenuCommands::OnRedo()
{
   if (!mProject->GetUndoManager()->RedoAvailable()) {
      wxMessageBox(_("Nothing to redo"));
      return;
   }
   // Can't redo whilst dragging
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   if (trackPanel->IsMouseCaptured()) {
      return;
   }

   TrackList *const l = mProject->GetUndoManager()->Redo(&mProject->GetViewInfo().selectedRegion);
   mProject->PopState(l);

   trackPanel->SetFocusedTrack(NULL);
   trackPanel->EnsureVisible(trackPanel->GetFirstSelectedTrack());

   mProject->RedrawProject();

   HistoryWindow *const historyWindow = mProject->GetHistoryWindow();
   if (historyWindow)
      historyWindow->UpdateDisplay();

   mProject->ModifyUndoMenuItems();
}
