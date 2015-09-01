#include "EditMenuCommands.h"

#include "../HistoryWindow.h"
#include "../LabelTrack.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../TrackPanel.h"
#include "../WaveTrack.h"
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

   c->AddSeparator();

   // Basic Edit coomands
   /* i18n-hint: (verb)*/
   c->AddItem(wxT("Cut"), _("Cu&t"), FN(OnCut), wxT("Ctrl+X"),
      AudioIONotBusyFlag | CutCopyAvailableFlag,
      AudioIONotBusyFlag | CutCopyAvailableFlag);
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

void EditMenuCommands::OnCut()
{
   TrackListIterator iter(mProject->GetTracks());
   Track *n = iter.First();
   Track *dest;

   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   ViewInfo &viewInfo = mProject->GetViewInfo();

   // This doesn't handle cutting labels, it handles
   // cutting the _text_ inside of labels, i.e. if you're
   // in the middle of editing the label text and select "Cut".

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Label) {
            if (((LabelTrack *)n)->CutSelectedText()) {
               trackPanel->Refresh(false);
               return;
            }
         }
      }
      n = iter.Next();
   }

   mProject->ClearClipboard();
   n = iter.First();
   while (n) {
      if (n->GetSelected()) {
         dest = NULL;
#if defined(USE_MIDI)
         if (n->GetKind() == Track::Note)
            // Since portsmf has a built-in cut operator, we use that instead
            n->Cut(viewInfo.selectedRegion.t0(),
            viewInfo.selectedRegion.t1(), &dest);
         else
#endif
            n->Copy(viewInfo.selectedRegion.t0(),
            viewInfo.selectedRegion.t1(), &dest);

         if (dest) {
            dest->SetChannel(n->GetChannel());
            dest->SetLinked(n->GetLinked());
            dest->SetName(n->GetName());
            AudacityProject::msClipboard->Add(dest);
         }
      }
      n = iter.Next();
   }

   n = iter.First();
   while (n) {
      // We clear from selected and sync-lock selected tracks.
      if (n->GetSelected() || n->IsSyncLockSelected()) {
         switch (n->GetKind())
         {
#if defined(USE_MIDI)
         case Track::Note:
            //if NoteTrack, it was cut, so do not clear anything
            break;
#endif
         case Track::Wave:
            if (gPrefs->Read(wxT("/GUI/EnableCutLines"), (long)0)) {
               ((WaveTrack*)n)->ClearAndAddCutLine(
                  viewInfo.selectedRegion.t0(),
                  viewInfo.selectedRegion.t1());
               break;
            }

            // Fall through

         default:
            n->Clear(viewInfo.selectedRegion.t0(),
               viewInfo.selectedRegion.t1());
            break;
         }
      }
      n = iter.Next();
   }

   AudacityProject::msClipT0 = viewInfo.selectedRegion.t0();
   AudacityProject::msClipT1 = viewInfo.selectedRegion.t1();
   AudacityProject::msClipProject = mProject;

   mProject->PushState(_("Cut to the clipboard"), _("Cut"));

   mProject->RedrawProject();

   viewInfo.selectedRegion.collapseToT0();
}
