#include "../Audacity.h"
#include "EditMenuCommands.h"

#include "../HistoryWindow.h"
#include "../MixerBoard.h"
#include "../LabelTrack.h"
#include "../NoteTrack.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../TimeTrack.h"
#include "../TrackPanel.h"
#include "../UndoManager.h"
#include "../WaveTrack.h"
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
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);
   /* i18n-hint: (verb)*/
   c->AddItem(wxT("Duplicate"), _("Duplic&ate"), FN(OnDuplicate), wxT("Ctrl+D"));

   c->AddSeparator();

   c->BeginSubMenu(_("R&emove Special"));
   {
      /* i18n-hint: (verb) Do a special kind of cut*/
      c->AddItem(wxT("SplitCut"), _("Spl&it Cut"), FN(OnSplitCut), wxT("Ctrl+Alt+X"));
      /* i18n-hint: (verb) Do a special kind of DELETE*/
      c->AddItem(wxT("SplitDelete"), _("Split D&elete"), FN(OnSplitDelete), wxT("Ctrl+Alt+K"),
         AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag);

      c->AddSeparator();

      /* i18n-hint: (verb)*/
      c->AddItem(wxT("Silence"), _("Silence Audi&o"), FN(OnSilence), wxT("Ctrl+L"),
         AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag);
      /* i18n-hint: (verb)*/
      c->AddItem(wxT("Trim"), _("Tri&m Audio"), FN(OnTrim), wxT("Ctrl+T"),
         AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag);
   }
   c->EndSubMenu();

   c->AddItem(wxT("PasteNewLabel"), _("Paste Te&xt to New Label"), FN(OnPasteNewLabel), wxT("Ctrl+Alt+V"),
      AudioIONotBusyFlag, AudioIONotBusyFlag);
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

   const auto &state = mProject->GetUndoManager()->Redo(&mProject->GetViewInfo().selectedRegion);
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

void EditMenuCommands::OnCut()
{
   TrackListIterator iter(mProject->GetTracks());
   Track *n = iter.First();

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
         Track::Holder dest;
#if defined(USE_MIDI)
         if (n->GetKind() == Track::Note)
            // Since portsmf has a built-in cut operator, we use that instead
            dest = n->Cut(viewInfo.selectedRegion.t0(),
                          viewInfo.selectedRegion.t1());
         else
#endif
            dest = n->Copy(viewInfo.selectedRegion.t0(),
                           viewInfo.selectedRegion.t1());

         if (dest) {
            dest->SetChannel(n->GetChannel());
            dest->SetLinked(n->GetLinked());
            dest->SetName(n->GetName());
            AudacityProject::msClipboard->Add(std::move(dest));
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

   auto history = mProject->GetHistoryWindow();
   if (history)
      history->UpdateDisplay();
}

void EditMenuCommands::OnDelete()
{
   mProject->Clear();
}

void EditMenuCommands::OnCopy()
{

   TrackListIterator iter(mProject->GetTracks());

   Track *n = iter.First();

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
         auto dest = n->Copy(viewInfo.selectedRegion.t0(),
                             viewInfo.selectedRegion.t1());
         if (dest) {
            dest->SetChannel(n->GetChannel());
            dest->SetLinked(n->GetLinked());
            dest->SetName(n->GetName());
            AudacityProject::msClipboard->Add(std::move(dest));
         }
      }
      n = iter.Next();
   }

   AudacityProject::msClipT0 = viewInfo.selectedRegion.t0();
   AudacityProject::msClipT1 = viewInfo.selectedRegion.t1();
   AudacityProject::msClipProject = mProject;

   //Make sure the menus/toolbar states get updated
   trackPanel->Refresh(false);

   auto history = mProject->GetHistoryWindow();
   if (history)
      history->UpdateDisplay();
}

void EditMenuCommands::OnPaste()
{
   // Handle text paste (into active label) first.
   if (this->HandlePasteText())
      return;

   // If nothing's selected, we just insert NEW tracks.
   if (this->HandlePasteNothingSelected())
      return;

   // Otherwise, paste into the selected tracks.
   double t0 = mProject->mViewInfo.selectedRegion.t0();
   double t1 = mProject->mViewInfo.selectedRegion.t1();

   TrackListIterator iter(mProject->GetTracks());
   TrackListConstIterator clipIter(AudacityProject::msClipboard.get());

   Track *n = iter.First();
   const Track *c = clipIter.First();
   if (c == NULL)
      return;
   Track *ff = NULL;
   const Track *tmpSrc = NULL;
   const Track *tmpC = NULL;
   const Track *prev = NULL;

   bool bAdvanceClipboard = true;
   bool bPastedSomething = false;
   bool bTrackTypeMismatch = false;

   while (n && c) {
      if (n->GetSelected()) {
         bAdvanceClipboard = true;
         if (tmpC)
            c = tmpC;
         if (c->GetKind() != n->GetKind()) {
            if (!bTrackTypeMismatch) {
               tmpSrc = prev;
               tmpC = c;
            }
            bTrackTypeMismatch = true;
            bAdvanceClipboard = false;
            c = tmpSrc;

            // If the types still don't match...
            while (c && c->GetKind() != n->GetKind()) {
               prev = c;
               c = clipIter.Next();
            }
         }

         // Handle case where the first track in clipboard
         // is of different type than the first selected track
         if (!c) {
            c = tmpC;
            while (n && (c->GetKind() != n->GetKind() || !n->GetSelected()))
            {
               // Must perform sync-lock adjustment before incrementing n
               if (n->IsSyncLockSelected()) {
                  bPastedSomething |= n->SyncLockAdjust(t1, t0+(AudacityProject::msClipT1 - AudacityProject::msClipT0));
               }
               n = iter.Next();
            }
            if (!n)
               c = NULL;
         }

         // The last possible case for cross-type pastes: triggered when we try to
         // paste 1+ tracks from one type into 1+ tracks of another type. If
         // there's a mix of types, this shouldn't run.
         if (!c) {
            ::wxMessageBox(
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
            ::wxMessageBox(
               _("Copying stereo audio into a mono track is not allowed."),
               _("Error"), wxICON_ERROR, mProject);
            break;
         }

         if (!ff)
            ff = n;

         Maybe<WaveTrack::Locker> locker;
         if (AudacityProject::msClipProject != mProject && c->GetKind() == Track::Wave)
            locker.create(static_cast<const WaveTrack*>(c));

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
               ((LabelTrack *)n)->ShiftLabelsOnInsert(AudacityProject::msClipT1 - AudacityProject::msClipT0, t0);

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
               bPastedSomething |= ((WaveTrack *)n)->ClearAndPaste(t0, t1, c, true, true);
            }
            else
            {
               n->Clear(t0, t1);
               bPastedSomething |= n->Paste(t0, c);
            }
         }

         if (bAdvanceClipboard){
            prev = c;
            c = clipIter.Next();
         }
      } // if (n->GetSelected())
      else if (n->IsSyncLockSelected())
      {
         bPastedSomething |=  n->SyncLockAdjust(t1, t0 + AudacityProject::msClipT1 - AudacityProject::msClipT0);
      }

      n = iter.Next();
   }

   // This block handles the cases where our clipboard is smaller
   // than the amount of selected destination tracks. We take the
   // last wave track, and paste that one into the remaining
   // selected tracks.
   if ( n && !c )
   {
      TrackListOfKindIterator clipWaveIter(Track::Wave, AudacityProject::msClipboard.get());
      c = clipWaveIter.Last();

      while (n) {
         if (n->GetSelected() && n->GetKind()==Track::Wave) {
            if (c && c->GetKind() == Track::Wave) {
               bPastedSomething |=
                  ((WaveTrack *)n)->ClearAndPaste(t0, t1, (WaveTrack *)c, true, true);
            }
            else {
               auto tmp = mProject->GetTrackFactory()->NewWaveTrack( ((WaveTrack*)n)->GetSampleFormat(), ((WaveTrack*)n)->GetRate());
               bool bResult = tmp->InsertSilence(0.0, AudacityProject::msClipT1 - AudacityProject::msClipT0); // MJS: Is this correct?
               wxASSERT(bResult); // TO DO: Actually handle this.
               wxUnusedVar(bResult);
               tmp->Flush();

               bPastedSomething |=
                  ((WaveTrack *)n)->ClearAndPaste(t0, t1, tmp.get(), true, true);
            }
         }
         else if (n->GetKind() == Track::Label && n->GetSelected())
         {
            ((LabelTrack *)n)->Clear(t0, t1);

            // As above, only shift labels if sync-lock is on.
            if (mProject->IsSyncLocked())
               ((LabelTrack *)n)->ShiftLabelsOnInsert(AudacityProject::msClipT1 - AudacityProject::msClipT0, t0);
         }
         else if (n->IsSyncLockSelected())
         {
            n->SyncLockAdjust(t1, t0 + AudacityProject::msClipT1 - AudacityProject::msClipT0);
         }

         n = iter.Next();
      }
   }

   // TODO: What if we clicked past the end of the track?

   if (bPastedSomething)
   {
      mProject->mViewInfo.selectedRegion.setT1(t0 + AudacityProject::msClipT1 - AudacityProject::msClipT0);

      mProject->PushState(_("Pasted from the clipboard"), _("Paste"));

      mProject->RedrawProject();

      if (ff)
         mProject->GetTrackPanel()->EnsureVisible(ff);
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
   TrackListOfKindIterator iterLabelTrack(Track::Label, mProject->GetTracks());
   LabelTrack* pLabelTrack = (LabelTrack*)(iterLabelTrack.First());
   while (pLabelTrack)
   {
      // Does this track have an active label?
      if (pLabelTrack->IsSelected()) {

         // Yes, so try pasting into it
         if (pLabelTrack->PasteSelectedText(mProject->mViewInfo.selectedRegion.t0(),
                                            mProject->mViewInfo.selectedRegion.t1()))
         {
            mProject->PushState(_("Pasted text from the clipboard"), _("Paste"));

            // Make sure caret is in view
            int x;
            if (pLabelTrack->CalcCursorX(&x)) {
               mProject->GetTrackPanel()->ScrollIntoView(x);
            }

            // Redraw everyting (is that necessary???) and bail
            mProject->RedrawProject();
            return true;
         }
      }
      pLabelTrack = (LabelTrack *) iterLabelTrack.Next();
   }
   return false;
}

// Return true if nothing selected, regardless of paste result.
// If nothing was selected, create and paste into NEW tracks.
// (This was formerly the second part of overly-long OnPaste.)
bool EditMenuCommands::HandlePasteNothingSelected()
{
   // First check whether anything's selected.
   bool bAnySelected = false;
   TrackListIterator iterTrack(mProject->GetTracks());
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
      TrackListConstIterator iterClip(AudacityProject::msClipboard.get());
      auto pClip = iterClip.First();
      if (!pClip)
         return true; // nothing to paste

      Track::Holder pNewTrack;
      Track* pFirstNewTrack = NULL;
      while (pClip) {
         Maybe<WaveTrack::Locker> locker;
         if ((AudacityProject::msClipProject != mProject) && (pClip->GetKind() == Track::Wave))
            locker.create(static_cast<const WaveTrack*>(pClip));

         switch (pClip->GetKind()) {
         case Track::Wave:
            {
               WaveTrack *w = (WaveTrack *)pClip;
               pNewTrack = mProject->GetTrackFactory()->NewWaveTrack(w->GetSampleFormat(), w->GetRate());
            }
            break;
         #ifdef USE_MIDI
            case Track::Note:
               pNewTrack = mProject->GetTrackFactory()->NewNoteTrack();
               break;
            #endif // USE_MIDI
         case Track::Label:
            pNewTrack = mProject->GetTrackFactory()->NewLabelTrack();
            break;
         case Track::Time:
            pNewTrack = mProject->GetTrackFactory()->NewTimeTrack();
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
         wxUnusedVar(bResult);

         if (!pFirstNewTrack)
            pFirstNewTrack = pNewTrack.get();

         pNewTrack->SetSelected(true);
         mProject->GetTracks()->Add(std::move(pNewTrack));

         pClip = iterClip.Next();
      }

      // Select some pasted samples, which is probably impossible to get right
      // with various project and track sample rates.
      // So do it at the sample rate of the project
      AudacityProject *p = GetActiveProject();
      double projRate = p->GetRate();
      double quantT0 = QUANTIZED_TIME(AudacityProject::msClipT0, projRate);
      double quantT1 = QUANTIZED_TIME(AudacityProject::msClipT1, projRate);
      mProject->mViewInfo.selectedRegion.setTimes(
         0.0,   // anywhere else and this should be
                // half a sample earlier
         quantT1 - quantT0);

      mProject->PushState(_("Pasted from the clipboard"), _("Paste"));

      mProject->RedrawProject();

      if (pFirstNewTrack)
         mProject->GetTrackPanel()->EnsureVisible(pFirstNewTrack);

      return true;
   }
}

void EditMenuCommands::OnDuplicate()
{
   TrackList *const trackList = mProject->GetTracks();
   TrackListIterator iter(trackList);

   ViewInfo &viewInfo = mProject->GetViewInfo();

   Track *l = iter.Last();
   Track *n = iter.First();

   while (n) {
      if (n->GetSelected()) {
         auto dest = n->Copy(viewInfo.selectedRegion.t0(),
                             viewInfo.selectedRegion.t1());
         if (dest) {
            dest->Init(*n);
            dest->SetOffset(wxMax(viewInfo.selectedRegion.t0(), n->GetOffset()));
            trackList->Add(std::move(dest));
         }
      }

      if (n == l) {
         break;
      }

      n = iter.Next();
   }

   mProject->PushState(_("Duplicated"), _("Duplicate"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnSplitCut()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();

   TrackListIterator iter(mProject->GetTracks());
   Track *n = iter.First();

   mProject->ClearClipboard();
   while (n) {
      if (n->GetSelected()) {
         Track::Holder dest;
         if (n->GetKind() == Track::Wave)
         {
            dest = ((WaveTrack*)n)->SplitCut(
               viewInfo.selectedRegion.t0(),
               viewInfo.selectedRegion.t1());
         }
         else
         {
            dest = n->Copy(viewInfo.selectedRegion.t0(),
               viewInfo.selectedRegion.t1());
            n->Silence(viewInfo.selectedRegion.t0(),
               viewInfo.selectedRegion.t1());
         }
         if (dest) {
            dest->SetChannel(n->GetChannel());
            dest->SetLinked(n->GetLinked());
            dest->SetName(n->GetName());
            AudacityProject::msClipboard->Add(std::move(dest));
         }
      }
      n = iter.Next();
   }

   AudacityProject::msClipT0 = viewInfo.selectedRegion.t0();
   AudacityProject::msClipT1 = viewInfo.selectedRegion.t1();
   AudacityProject::msClipProject = mProject;

   mProject->PushState(_("Split-cut to the clipboard"), _("Split Cut"));

   mProject->RedrawProject();

   auto history = mProject->GetHistoryWindow();
   if (history)
      history->UpdateDisplay();
}

void EditMenuCommands::OnSplitDelete()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();

   TrackListIterator iter(mProject->GetTracks());

   Track *n = iter.First();

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Wave)
         {
            ((WaveTrack*)n)->SplitDelete(viewInfo.selectedRegion.t0(),
               viewInfo.selectedRegion.t1());
         }
         else {
            n->Silence(viewInfo.selectedRegion.t0(),
               viewInfo.selectedRegion.t1());
         }
      }
      n = iter.Next();
   }

   mProject->PushState(wxString::Format(_("Split-deleted %.2f seconds at t=%.2f"),
      viewInfo.selectedRegion.duration(),
      viewInfo.selectedRegion.t0()),
      _("Split Delete"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnSilence()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();

   SelectedTrackListOfKindIterator iter(Track::Wave, mProject->GetTracks());

   for (Track *n = iter.First(); n; n = iter.Next())
      n->Silence(viewInfo.selectedRegion.t0(), viewInfo.selectedRegion.t1());

   mProject->PushState(wxString::
      Format(_("Silenced selected tracks for %.2f seconds at %.2f"),
      viewInfo.selectedRegion.duration(),
      viewInfo.selectedRegion.t0()),
      _("Silence"));

   mProject->GetTrackPanel()->Refresh(false);
}

void EditMenuCommands::OnTrim()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();

   if (viewInfo.selectedRegion.isPoint())
      return;

   TrackListIterator iter(mProject->GetTracks());
   Track *n = iter.First();

   while (n) {
      if (n->GetSelected()) {
         switch (n->GetKind())
         {
#if defined(USE_MIDI)
         case Track::Note:
            ((NoteTrack*)n)->Trim(viewInfo.selectedRegion.t0(),
               viewInfo.selectedRegion.t1());
            break;
#endif

         case Track::Wave:
            //Delete the section before the left selector
            ((WaveTrack*)n)->Trim(viewInfo.selectedRegion.t0(),
               viewInfo.selectedRegion.t1());
            break;

         default:
            break;
         }
      }
      n = iter.Next();
   }

   mProject->PushState(wxString::Format(_("Trim selected audio tracks from %.2f seconds to %.2f seconds"),
      viewInfo.selectedRegion.t0(), viewInfo.selectedRegion.t1()),
      _("Trim Audio"));

   mProject->RedrawProject();
}

// Creates a new label in each selected label track with text from the system
// clipboard
void EditMenuCommands::OnPasteNewLabel()
{
   bool bPastedSomething = false;

   TrackList *const trackList = mProject->GetTracks();
   SelectedTrackListOfKindIterator iter(Track::Label, trackList);
   Track *t = iter.First();
   if (!t)
   {
      // If there are no selected label tracks, try to choose the first label
      // track after some other selected track
      TrackListIterator iter1(trackList);
      for (Track *t1 = iter1.First(); t1; t1 = iter1.Next()) {
         if (t1->GetSelected()) {
            // Look for a label track
            while (0 != (t1 = iter1.Next())) {
               if (t1->GetKind() == Track::Label) {
                  t = t1;
                  break;
               }
            }
            if (t) break;
         }
      }

      // If no match found, add one
      if (!t) {
         t = mProject->GetTracks()->Add(
            mProject->GetTrackFactory()->NewLabelTrack());
      }

      // Select this track so the loop picks it up
      t->SetSelected(true);
   }

   LabelTrack *plt = NULL; // the previous track
   for (Track *t = iter.First(); t; t = iter.Next())
   {
      LabelTrack *lt = (LabelTrack *)t;

      // Unselect the last label, so we'll have just one active label when
      // we're done
      if (plt)
         plt->Unselect();

      // Add a new label, paste into it
      // Paul L:  copy whatever defines the selected region, not just times
      ViewInfo &viewInfo = mProject->GetViewInfo();
      lt->AddLabel(viewInfo.selectedRegion);
      if (lt->PasteSelectedText(viewInfo.selectedRegion.t0(),
         viewInfo.selectedRegion.t1()))
         bPastedSomething = true;

      // Set previous track
      plt = lt;
   }

   // plt should point to the last label track pasted to -- ensure it's visible
   // and set focus
   if (plt) {
      TrackPanel *const trackPanel = mProject->GetTrackPanel();
      trackPanel->EnsureVisible(plt);
      trackPanel->SetFocus();
   }

   if (bPastedSomething) {
      mProject->PushState(_("Pasted from the clipboard"), _("Paste Text to New Label"));

      // Is this necessary? (carried over from former logic in OnPaste())
      mProject->RedrawProject();
   }
}
