#include "EditMenuCommands.h"

#include "../HistoryWindow.h"
#include "../LabelTrack.h"
#include "../NoteTrack.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../TimeTrack.h"
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
   c->AddItem(wxT("Delete"), _("&Delete"), FN(OnDelete), wxT("Ctrl+K"));
   /* i18n-hint: (verb)*/
   c->AddItem(wxT("Copy"), _("&Copy"), FN(OnCopy), wxT("Ctrl+C"),
      AudioIONotBusyFlag | CutCopyAvailableFlag,
      AudioIONotBusyFlag | CutCopyAvailableFlag);
   /* i18n-hint: (verb)*/
   c->AddItem(wxT("Paste"), _("&Paste"), FN(OnPaste), wxT("Ctrl+V"),
      AudioIONotBusyFlag | ClipboardFlag,
      AudioIONotBusyFlag | ClipboardFlag);
}

void EditMenuCommands::CreateNonMenuCommands(CommandManager *c)
{
   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddCommand(wxT("DeleteKey"), _("DeleteKey"), FN(OnDelete), wxT("Backspace"),
      AudioIONotBusyFlag | TracksSelectedFlag | TimeSelectedFlag,
      AudioIONotBusyFlag | TracksSelectedFlag | TimeSelectedFlag);

   c->AddCommand(wxT("DeleteKey2"), _("DeleteKey2"), FN(OnDelete), wxT("Delete"),
      AudioIONotBusyFlag | TracksSelectedFlag | TimeSelectedFlag,
      AudioIONotBusyFlag | TracksSelectedFlag | TimeSelectedFlag);
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

void EditMenuCommands::OnDelete()
{
   mProject->Clear();
}

void EditMenuCommands::OnCopy()
{

   TrackListIterator iter(mProject->GetTracks());

   Track *n = iter.First();
   Track *dest;

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Label) {
            if (((LabelTrack *)n)->CopySelectedText()) {
               //mTrackPanel->Refresh(false);
               return;
            }
         }
      }
      n = iter.Next();
   }

   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   ViewInfo &viewInfo = mProject->GetViewInfo();

   mProject->ClearClipboard();
   n = iter.First();
   while (n) {
      if (n->GetSelected()) {
         dest = NULL;
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

   AudacityProject::msClipT0 = viewInfo.selectedRegion.t0();
   AudacityProject::msClipT1 = viewInfo.selectedRegion.t1();
   AudacityProject::msClipProject = mProject;

   //Make sure the menus/toolbar states get updated
   trackPanel->Refresh(false);
}

void EditMenuCommands::OnPaste()
{
   // Handle text paste (into active label) first.
   if (this->HandlePasteText())
      return;

   // If nothing's selected, we just insert new tracks.
   if (HandlePasteNothingSelected())
      return;

   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   ViewInfo &viewInfo = mProject->GetViewInfo();

   // Otherwise, paste into the selected tracks.
   double t0 = viewInfo.selectedRegion.t0();
   double t1 = viewInfo.selectedRegion.t1();

   TrackListIterator iter(mProject->GetTracks());
   TrackListIterator clipIter(AudacityProject::msClipboard);

   Track *n = iter.First();
   Track *c = clipIter.First();
   if (c == NULL)
      return;
   Track *f = NULL;
   Track *tmpSrc = NULL;
   Track *tmpC = NULL;
   Track *prev = NULL;

   bool bAdvanceClipboard = true;
   bool bPastedSomething = false;
   bool bTrackTypeMismatch = false;

   while (n && c) {
      if (n->GetSelected()) {
         bAdvanceClipboard = true;
         if (tmpC) c = tmpC;
         if (c->GetKind() != n->GetKind()){
            if (!bTrackTypeMismatch) {
               tmpSrc = prev;
               tmpC = c;
            }
            bTrackTypeMismatch = true;
            bAdvanceClipboard = false;
            c = tmpSrc;

            // If the types still don't match...
            while (c && c->GetKind() != n->GetKind()){
               prev = c;
               c = clipIter.Next();
            }
         }

         // Handle case where the first track in clipboard
         // is of different type than the first selected track
         if (!c){
            c = tmpC;
            while (n && (c->GetKind() != n->GetKind() || !n->GetSelected()))
            {
               // Must perform sync-lock adjustment before incrementing n
               if (n->IsSyncLockSelected()) {
                  bPastedSomething |=
                     n->SyncLockAdjust(t1,
                        t0 + (AudacityProject::msClipT1 -
                              AudacityProject::msClipT0));
               }
               n = iter.Next();
            }
            if (!n) c = NULL;
         }

         // The last possible case for cross-type pastes: triggered when we try to
         // paste 1+ tracks from one type into 1+ tracks of another type. If
         // there's a mix of types, this shouldn't run.
         if (!c){
            wxMessageBox(
               _("Pasting one type of track into another is not allowed."),
               _("Error"), wxICON_ERROR, mProject);
            c = n;//so we don't trigger any !c conditions on our way out
            break;
         }

         // When trying to copy from stereo to mono track, show error and exit
         // TODO: Automatically offer user to mix down to mono (unfortunately
         //       this is not easy to implement
         if (c->GetLinked() && !n->GetLinked())
         {
            wxMessageBox(
               _("Copying stereo audio into a mono track is not allowed."),
               _("Error"), wxICON_ERROR, mProject);
            break;
         }

         if (!f)
            f = n;

         if (AudacityProject::msClipProject != mProject && c->GetKind() == Track::Wave)
            ((WaveTrack *)c)->Lock();

         if (c->GetKind() == Track::Wave && n && n->GetKind() == Track::Wave)
         {
            bPastedSomething |=
               ((WaveTrack*)n)->ClearAndPaste(t0, t1, (WaveTrack*)c, true, true);
         }
         else if (c->GetKind() == Track::Label &&
            n && n->GetKind() == Track::Label)
         {
            ((LabelTrack *)n)->Clear(t0, t1);

            // To be (sort of) consistent with Clear behavior, we'll only shift
            // them if sync-lock is on.
            if (mProject->IsSyncLocked())
               ((LabelTrack *)n)->
               ShiftLabelsOnInsert(AudacityProject::msClipT1 -
                                   AudacityProject::msClipT0, t0);

            bPastedSomething |= ((LabelTrack *)n)->PasteOver(t0, c);
         }
         else
         {
            bPastedSomething |= n->Paste(t0, c);
         }

         // When copying from mono to stereo track, paste the wave form
         // to both channels
         if (n->GetLinked() && !c->GetLinked())
         {
            n = iter.Next();

            if (n->GetKind() == Track::Wave) {
               //printf("Checking to see if we need to pre-clear the track\n");
               if (!((WaveTrack *)n)->IsEmpty(t0, t1)) {
                  ((WaveTrack *)n)->Clear(t0, t1);
               }
               bPastedSomething |= ((WaveTrack *)n)->Paste(t0, c);
            }
            else
            {
               n->Clear(t0, t1);
               bPastedSomething |= n->Paste(t0, c);
            }
         }

         if (AudacityProject::msClipProject != mProject && c->GetKind() == Track::Wave)
            ((WaveTrack *)c)->Unlock();

         if (bAdvanceClipboard){
            prev = c;
            c = clipIter.Next();
         }
      } // if (n->GetSelected())
      else if (n->IsSyncLockSelected())
      {
         bPastedSomething |=
            n->SyncLockAdjust(t1, t0 + AudacityProject::msClipT1 -
                                       AudacityProject::msClipT0);
      }

      n = iter.Next();
   }

   // This block handles the cases where our clipboard is smaller
   // than the amount of selected destination tracks. We take the
   // last wave track, and paste that one into the remaining
   // selected tracks.
   if (n && !c)
   {
      TrackListOfKindIterator clipWaveIter(Track::Wave, AudacityProject::msClipboard);
      c = clipWaveIter.Last();

      while (n){
         if (n->GetSelected() && n->GetKind() == Track::Wave){
            if (c && c->GetKind() == Track::Wave){
               bPastedSomething |=
                  ((WaveTrack *)n)->ClearAndPaste(t0, t1, (WaveTrack *)c, true, true);
            }
            else{
               WaveTrack *tmp;
               tmp =
                  mProject->GetTrackFactory()->NewWaveTrack(
                     ((WaveTrack*)n)->GetSampleFormat(),
                     ((WaveTrack*)n)->GetRate());
               bool bResult =
                  tmp->InsertSilence(0.0, AudacityProject::msClipT1 -
                                          AudacityProject::msClipT0); // MJS: Is this correct?
               wxASSERT(bResult); // TO DO: Actually handle this.
               tmp->Flush();

               bPastedSomething |=
                  ((WaveTrack *)n)->ClearAndPaste(t0, t1, tmp, true, true);

               delete tmp;
            }
         }
         else if (n->GetKind() == Track::Label && n->GetSelected())
         {
            ((LabelTrack *)n)->Clear(t0, t1);

            // As above, only shift labels if sync-lock is on.
            if (mProject->IsSyncLocked())
               ((LabelTrack *)n)->
               ShiftLabelsOnInsert(AudacityProject::msClipT1 -
                                   AudacityProject::msClipT0, t0);
         }
         else if (n->IsSyncLockSelected())
         {
            n->SyncLockAdjust(t1, t0 + AudacityProject::msClipT1 -
                                       AudacityProject::msClipT0);
         }

         n = iter.Next();
      }
   }

   // TODO: What if we clicked past the end of the track?

   if (bPastedSomething)
   {
      viewInfo.selectedRegion.setT1(t0 + AudacityProject::msClipT1 -
                                         AudacityProject::msClipT0);

      mProject->PushState(_("Pasted from the clipboard"), _("Paste"));

      mProject->RedrawProject();

      if (f)
         trackPanel->EnsureVisible(f);
   }
}

void EditMenuCommands::OnPasteOver() // not currently in use it appears
{
   if ((AudacityProject::msClipT1 - AudacityProject::msClipT0) > 0.0)
   {
      ViewInfo &viewInfo = mProject->GetViewInfo();
      viewInfo.selectedRegion.setT1(
         viewInfo.selectedRegion.t0() + (AudacityProject::msClipT1 -
                                          AudacityProject::msClipT0));
      // MJS: pointless, given what we do in OnPaste?
   }
   OnPaste();

   return;
}

// Handle text paste (into active label), if any. Return true if did paste.
// (This was formerly the first part of overly-long OnPaste.)
bool EditMenuCommands::HandlePasteText()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   TrackListOfKindIterator iterLabelTrack(Track::Label, mProject->GetTracks());
   LabelTrack* pLabelTrack = (LabelTrack*)(iterLabelTrack.First());
   while (pLabelTrack)
   {
      // Does this track have an active label?
      if (pLabelTrack->IsSelected()) {

         // Yes, so try pasting into it
         if (pLabelTrack->PasteSelectedText(viewInfo.selectedRegion.t0(),
            viewInfo.selectedRegion.t1()))
         {
            mProject->PushState(_("Pasted text from the clipboard"), _("Paste"));

            // Make sure caret is in view
            int x;
            if (pLabelTrack->CalcCursorX(mProject, &x)) {
               mProject->GetTrackPanel()->ScrollIntoView(x);
            }

            // Redraw everyting (is that necessary???) and bail
            mProject->RedrawProject();
            return true;
         }
      }
      pLabelTrack = (LabelTrack *)iterLabelTrack.Next();
   }
   return false;
}

// Return true if nothing selected, regardless of paste result.
// If nothing was selected, create and paste into new tracks.
// (This was formerly the second part of overly-long OnPaste.)
bool EditMenuCommands::HandlePasteNothingSelected()
{
   // First check whether anything's selected.
   bool bAnySelected = false;
   TrackList *const trackList = mProject->GetTracks();
   TrackListIterator iterTrack(trackList);
   Track* pTrack = iterTrack.First();
   while (pTrack) {
      if (pTrack->GetSelected())
      {
         bAnySelected = true;
         break;
      }
      pTrack = iterTrack.Next();
   }

   if (bAnySelected)
      return false;
   else
   {
      TrackFactory *const trackFactory = mProject->GetTrackFactory();
      TrackPanel *const trackPanel = mProject->GetTrackPanel();
      ViewInfo &viewInfo = mProject->GetViewInfo();
      TrackListIterator iterClip(AudacityProject::msClipboard);
      Track* pClip = iterClip.First();
      if (!pClip)
         return true; // nothing to paste

      Track* pNewTrack;
      Track* pFirstNewTrack = NULL;
      while (pClip) {
         if ((AudacityProject::msClipProject != mProject) && (pClip->GetKind() == Track::Wave))
            ((WaveTrack*)pClip)->Lock();

         switch (pClip->GetKind()) {
         case Track::Wave:
         {
            WaveTrack *w = (WaveTrack *)pClip;
            pNewTrack = trackFactory->NewWaveTrack(w->GetSampleFormat(), w->GetRate());
         }
         break;
#ifdef USE_MIDI
         case Track::Note:
            pNewTrack = trackFactory->NewNoteTrack();
            break;
#endif // USE_MIDI
         case Track::Label:
            pNewTrack = trackFactory->NewLabelTrack();
            break;
         case Track::Time:
            pNewTrack = trackFactory->NewTimeTrack();
            break;
         default:
            pClip = iterClip.Next();
            continue;
         }
         wxASSERT(pClip);

         pNewTrack->SetLinked(pClip->GetLinked());
         pNewTrack->SetChannel(pClip->GetChannel());
         pNewTrack->SetName(pClip->GetName());

         bool bResult = pNewTrack->Paste(0.0, pClip);
         wxASSERT(bResult); // TO DO: Actually handle this.
         trackList->Add(pNewTrack);
         pNewTrack->SetSelected(true);

         if (AudacityProject::msClipProject != mProject && pClip->GetKind() == Track::Wave)
            ((WaveTrack *)pClip)->Unlock();

         if (!pFirstNewTrack)
            pFirstNewTrack = pNewTrack;

         pClip = iterClip.Next();
      }

      // Select some pasted samples, which is probably impossible to get right
      // with various project and track sample rates.
      // So do it at the sample rate of the project
      AudacityProject *p = GetActiveProject();
      double projRate = p->GetRate();
      double quantT0 = QUANTIZED_TIME(AudacityProject::msClipT0, projRate);
      double quantT1 = QUANTIZED_TIME(AudacityProject::msClipT1, projRate);
      viewInfo.selectedRegion.setTimes(
         0.0,   // anywhere else and this should be
         // half a sample earlier
         quantT1 - quantT0);

      mProject->PushState(_("Pasted from the clipboard"), _("Paste"));

      mProject->RedrawProject();

      if (pFirstNewTrack)
         trackPanel->EnsureVisible(pFirstNewTrack);

      return true;
   }
}
