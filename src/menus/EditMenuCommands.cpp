#include "../Audacity.h"
#include "EditMenuCommands.h"

#include <algorithm>

#include "../AudioIO.h"
#include "../HistoryWindow.h"
#include "../MixerBoard.h"
#include "../LabelTrack.h"
#include "../MixerBoard.h"
#include "../NoteTrack.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../TimeDialog.h"
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

   c->AddSeparator();

   /////////////////////////////////////////////////////////////////////////////

   c->BeginSubMenu(_("Clip B&oundaries"));
   {
      /* i18n-hint: (verb) It's an item on a menu. */
      c->AddItem(wxT("Split"), _("Sp&lit"), FN(OnSplit), wxT("Ctrl+I"),
         AudioIONotBusyFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | WaveTracksSelectedFlag);
      c->AddItem(wxT("SplitNew"), _("Split Ne&w"), FN(OnSplitNew), wxT("Ctrl+Alt+I"),
         AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag);
      c->AddSeparator();
      /* i18n-hint: (verb)*/
      c->AddItem(wxT("Join"), _("&Join"), FN(OnJoin), wxT("Ctrl+J"));
      c->AddItem(wxT("Disjoin"), _("Detac&h at Silences"), FN(OnDisjoin), wxT("Ctrl+Alt+J"));
   }
   c->EndSubMenu();

   c->AddSeparator();

   /////////////////////////////////////////////////////////////////////////////

   c->BeginSubMenu(_("La&beled Audio"));
   {
      c->SetDefaultFlags(AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag | TimeSelectedFlag,
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag | TimeSelectedFlag);

      /* i18n-hint: (verb)*/
      c->AddItem(wxT("CutLabels"), _("&Cut"), FN(OnCutLabels), wxT("Alt+X"),
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag | TimeSelectedFlag | IsNotSyncLockedFlag,
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag | TimeSelectedFlag | IsNotSyncLockedFlag);
      c->AddItem(wxT("DeleteLabels"), _("&Delete"), FN(OnDeleteLabels), wxT("Alt+K"),
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag | TimeSelectedFlag | IsNotSyncLockedFlag,
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag | TimeSelectedFlag | IsNotSyncLockedFlag);

      c->AddSeparator();

      /* i18n-hint: (verb) A special way to cut out a piece of audio*/
      c->AddItem(wxT("SplitCutLabels"), _("&Split Cut"), FN(OnSplitCutLabels), wxT("Alt+Shift+X"));
      c->AddItem(wxT("SplitDeleteLabels"), _("Sp&lit Delete"), FN(OnSplitDeleteLabels), wxT("Alt+Shift+K"));

      c->AddSeparator();


      c->AddItem(wxT("SilenceLabels"), _("Silence &Audio"), FN(OnSilenceLabels), wxT("Alt+L"));
      /* i18n-hint: (verb)*/
      c->AddItem(wxT("CopyLabels"), _("Co&py"), FN(OnCopyLabels), wxT("Alt+Shift+C"));

      c->AddSeparator();

      /* i18n-hint: (verb)*/
      c->AddItem(wxT("SplitLabels"), _("Spli&t"), FN(OnSplitLabels), wxT("Alt+I"),
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag,
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag);
      /* i18n-hint: (verb)*/
      c->AddItem(wxT("JoinLabels"), _("&Join"), FN(OnJoinLabels), wxT("Alt+J"));
      c->AddItem(wxT("DisjoinLabels"), _("Detac&h at Silences"), FN(OnDisjoinLabels), wxT("Alt+Shift+J"));
   }
   c->EndSubMenu();

   /////////////////////////////////////////////////////////////////////////////

   /* i18n-hint: (verb) It's an item on a menu. */
   c->BeginSubMenu(_("&Select"));
   {
      c->SetDefaultFlags(TracksExistFlag, TracksExistFlag);

      c->AddItem(wxT("SelectAll"), _("&All"), FN(OnSelectAll), wxT("Ctrl+A"));
      c->AddItem(wxT("SelectNone"), _("&None"), FN(OnSelectNone), wxT("Ctrl+Shift+A"));

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      c->BeginSubMenu(_("S&pectral"));
      c->AddItem(wxT("ToggleSpectralSelection"), _("To&ggle spectral selection"), FN(OnToggleSpectralSelection), wxT("Q"));
      c->AddItem(wxT("NextHigherPeakFrequency"), _("Next Higher Peak Frequency"), FN(OnNextHigherPeakFrequency));
      c->AddItem(wxT("NextLowerPeakFrequency"), _("Next Lower Peak Frequency"), FN(OnNextLowerPeakFrequency));
      c->EndSubMenu();
#endif

      c->AddItem(wxT("SetLeftSelection"), _("&Left at Playback Position"), FN(OnSetLeftSelection), wxT("["));
      c->AddItem(wxT("SetRightSelection"), _("&Right at Playback Position"), FN(OnSetRightSelection), wxT("]"));

      c->SetDefaultFlags(TracksSelectedFlag, TracksSelectedFlag);

      c->AddItem(wxT("SelStartCursor"), _("Track &Start to Cursor"), FN(OnSelectStartCursor), wxT("Shift+J"));
      c->AddItem(wxT("SelCursorEnd"), _("Cursor to Track &End"), FN(OnSelectCursorEnd), wxT("Shift+K"));
     c->AddItem(wxT("SelCursorStoredCursor"), _("Cursor to Stored &Cursor Position"), FN(OnSelectCursorStoredCursor),
        wxT(""), TracksExistFlag, TracksExistFlag);

      c->AddSeparator();

      c->AddItem(wxT("SelAllTracks"), _("In All &Tracks"), FN(OnSelectAllTracks),
         wxT("Ctrl+Shift+K"),
         TracksExistFlag, TracksExistFlag);

#ifdef EXPERIMENTAL_SYNC_LOCK
      c->AddItem(wxT("SelSyncLockTracks"), _("In All S&ync-Locked Tracks"),
         FN(OnSelectSyncLockSel), wxT("Ctrl+Shift+Y"),
         TracksSelectedFlag | IsSyncLockedFlag,
         TracksSelectedFlag | IsSyncLockedFlag);
#endif
   }
   c->EndSubMenu();


   /////////////////////////////////////////////////////////////////////////////

   c->AddItem(wxT("ZeroCross"), _("Find &Zero Crossings"), FN(OnZeroCrossing), wxT("Z"));

   /////////////////////////////////////////////////////////////////////////////

   c->BeginSubMenu(_("Mo&ve Cursor"));
   {
      c->AddItem(wxT("CursSelStart"), _("to Selection Star&t"), FN(OnCursorSelStart));
      c->AddItem(wxT("CursSelEnd"), _("to Selection En&d"), FN(OnCursorSelEnd));

      c->AddItem(wxT("CursTrackStart"), _("to Track &Start"), FN(OnCursorTrackStart), wxT("J"));
      c->AddItem(wxT("CursTrackEnd"), _("to Track &End"), FN(OnCursorTrackEnd), wxT("K"));
   }
   c->EndSubMenu();
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

   ClearClipboard();
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

   ClearClipboard();
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

   ClearClipboard();
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

// Creates a NEW label in each selected label track with text from the system
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

      // Add a NEW label, paste into it
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

void EditMenuCommands::OnSplit()
{
   TrackListIterator iter(mProject->GetTracks());

   double sel0 = mProject->mViewInfo.selectedRegion.t0();
   double sel1 = mProject->mViewInfo.selectedRegion.t1();

   for (Track* n=iter.First(); n; n = iter.Next())
   {
      if (n->GetKind() == Track::Wave)
      {
         WaveTrack* wt = (WaveTrack*)n;
         if (wt->GetSelected())
            wt->Split( sel0, sel1 );
      }
   }

   mProject->PushState(_("Split"), _("Split"));
   mProject->GetTrackPanel()->Refresh(false);
#if 0
//ANSWER-ME: Do we need to keep this commented out OnSplit() code?
// This whole section no longer used...
   /*
    * Previous (pre-multiclip) implementation of "Split" command
    * This does work only when a range is selected!
    *
   TrackListIterator iter(mTracks);

   Track *n = iter.First();
   Track *dest;

   TrackList newTracks;

   while (n) {
      if (n->GetSelected()) {
         double sel0 = mViewInfo.selectedRegion.t0();
         double sel1 = mViewInfo.selectedRegion.t1();

         dest = NULL;
         n->Copy(sel0, sel1, &dest);
         if (dest) {
            dest->Init(*n);
            dest->SetOffset(wxMax(sel0, n->GetOffset()));

            if (sel1 >= n->GetEndTime())
               n->Clear(sel0, sel1);
            else if (sel0 <= n->GetOffset()) {
               n->Clear(sel0, sel1);
               n->SetOffset(sel1);
            } else
               n->Silence(sel0, sel1);

            newTracks.Add(dest);
         }
      }
      n = iter.Next();
   }

   TrackListIterator nIter(&newTracks);
   n = nIter.First();
   while (n) {
      mTracks->Add(n);
      n = nIter.Next();
   }

   PushState(_("Split"), _("Split"));

   FixScrollbars();
   mTrackPanel->Refresh(false);
   */
#endif
}

void EditMenuCommands::OnSplitNew()
{
   TrackListIterator iter(mProject->GetTracks());
   Track *l = iter.Last();

   for (Track *n = iter.First(); n; n = iter.Next()) {
      if (n->GetSelected()) {
         Track::Holder dest;
         double newt0 = 0, newt1 = 0;
         double offset = n->GetOffset();
         if (n->GetKind() == Track::Wave) {
            const auto wt = static_cast<WaveTrack*>(n);
            // Clips must be aligned to sample positions or the NEW clip will not fit in the gap where it came from
            offset = wt->LongSamplesToTime(wt->TimeToLongSamples(offset));
            newt0 = wt->LongSamplesToTime(wt->TimeToLongSamples(mProject->mViewInfo.selectedRegion.t0()));
            newt1 = wt->LongSamplesToTime(wt->TimeToLongSamples(mProject->mViewInfo.selectedRegion.t1()));
            dest = wt->SplitCut(newt0, newt1);
         }
#if 0
         // LL:  For now, just skip all non-wave tracks since the other do not
         //      yet support proper splitting.
         else {
            dest = n->Cut(mViewInfo.selectedRegion.t0(),
                          mViewInfo.selectedRegion.t1());
         }
#endif
         if (dest) {
            dest->SetChannel(n->GetChannel());
            dest->SetLinked(n->GetLinked());
            dest->SetName(n->GetName());
            dest->SetOffset(wxMax(newt0, offset));
            mProject->GetTracks()->Add(std::move(dest));
         }
      }

      if (n == l) {
         break;
      }
   }

   mProject->PushState(_("Split to new track"), _("Split New"));

   mProject->RedrawProject();
}


void EditMenuCommands::OnJoin()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   TrackListIterator iter(mProject->GetTracks());

   Track *n = iter.First();

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Wave)
         {
            ((WaveTrack*)n)->Join(viewInfo.selectedRegion.t0(),
                                  viewInfo.selectedRegion.t1());
         }
      }
      n = iter.Next();
   }

   mProject->PushState(wxString::Format(_("Joined %.2f seconds at t=%.2f"),
                              viewInfo.selectedRegion.duration(),
                              viewInfo.selectedRegion.t0()),
             _("Join"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnDisjoin()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   TrackListIterator iter(mProject->GetTracks());

   Track *n = iter.First();

   while (n) {
      if (n->GetSelected()) {
         if (n->GetKind() == Track::Wave)
         {
            ((WaveTrack*)n)->Disjoin(viewInfo.selectedRegion.t0(),
                                     viewInfo.selectedRegion.t1());
         }
      }
      n = iter.Next();
   }

   mProject->PushState(wxString::Format(_("Detached %.2f seconds at t=%.2f"),
                              viewInfo.selectedRegion.duration(),
                              viewInfo.selectedRegion.t0()),
             _("Detach"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnCutLabels()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   // Because of grouping the copy may need to operate on different tracks than
   // the clear, so we do these actions separately.
   EditClipboardByLabel(&WaveTrack::CopyNonconst);

   if (gPrefs->Read(wxT("/GUI/EnableCutLines"), (long)0))
      EditByLabel(&WaveTrack::ClearAndAddCutLine, true);
   else
      EditByLabel(&WaveTrack::Clear, true);

   AudacityProject::msClipProject = mProject;

   viewInfo.selectedRegion.collapseToT0();

   mProject->PushState(
      /* i18n-hint: (verb) past tense.  Audacity has just cut the labeled audio regions.*/
      _("Cut labeled audio regions to clipboard"),
      /* i18n-hint: (verb)*/
      _("Cut Labeled Audio"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnDeleteLabels()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   EditByLabel(&WaveTrack::Clear, true);

   viewInfo.selectedRegion.collapseToT0();

   mProject->PushState(
      /* i18n-hint: (verb) Audacity has just deleted the labeled audio regions*/
      _("Deleted labeled audio regions"),
      /* i18n-hint: (verb)*/
      _("Delete Labeled Audio"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnSplitCutLabels()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   EditClipboardByLabel(&WaveTrack::SplitCut);

   AudacityProject::msClipProject = mProject;

   mProject->PushState(
      /* i18n-hint: (verb) Audacity has just split cut the labeled audio regions*/
      _("Split Cut labeled audio regions to clipboard"),
      /* i18n-hint: (verb) Do a special kind of cut on the labels*/
      _("Split Cut Labeled Audio"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnSplitDeleteLabels()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   EditByLabel(&WaveTrack::SplitDelete, false);

   mProject->PushState(
      /* i18n-hint: (verb) Audacity has just done a special kind of DELETE on the labeled audio regions */
      _("Split Deleted labeled audio regions"),
      /* i18n-hint: (verb) Do a special kind of DELETE on labeled audio regions*/
      _("Split Delete Labeled Audio"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnSilenceLabels()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   EditByLabel(&WaveTrack::Silence, false);

   mProject->PushState(
      /* i18n-hint: (verb)*/
      _("Silenced labeled audio regions"),
      /* i18n-hint: (verb)*/
      _("Silence Labeled Audio"));

   mProject->GetTrackPanel()->Refresh(false);
}

void EditMenuCommands::OnCopyLabels()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   EditClipboardByLabel(&WaveTrack::CopyNonconst);

   AudacityProject::msClipProject = mProject;

   mProject->PushState(_("Copied labeled audio regions to clipboard"),
      /* i18n-hint: (verb)*/
      _("Copy Labeled Audio"));

   mProject->GetTrackPanel()->Refresh(false);
}

void EditMenuCommands::OnSplitLabels()
{
   EditByLabel(&WaveTrack::Split, false);

   mProject->PushState(
      /* i18n-hint: (verb) past tense.  Audacity has just split the labeled audio (a point or a region)*/
      _("Split labeled audio (points or regions)"),
      /* i18n-hint: (verb)*/
      _("Split Labeled Audio"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnJoinLabels()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   EditByLabel(&WaveTrack::Join, false);

   mProject->PushState(
      /* i18n-hint: (verb) Audacity has just joined the labeled audio (points or regions)*/
      _("Joined labeled audio (points or regions)"),
      /* i18n-hint: (verb)*/
      _("Join Labeled Audio"));

   mProject->RedrawProject();
}

void EditMenuCommands::OnDisjoinLabels()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   EditByLabel(&WaveTrack::Disjoin, false);

   mProject->PushState(
      /* i18n-hint: (verb) Audacity has just detached the labeled audio regions.
      This message appears in history and tells you about something
      Audacity has done.*/
      _("Detached labeled audio regions"),
      /* i18n-hint: (verb)*/
      _("Detach Labeled Audio"));

   mProject->RedrawProject();
}


//Executes the edit function on all selected wave tracks with
//regions specified by selected labels
//If No tracks selected, function is applied on all tracks
//If the function replaces the selection with audio of a different length,
// bSyncLockedTracks should be set true to perform the same action on sync-lock selected
// tracks.
void EditMenuCommands::EditByLabel( EditFunction action,
                                   bool bSyncLockedTracks )
{
   Regions regions;

   GetRegionsByLabel( regions );
   if( regions.size() == 0 )
      return;

   TrackListIterator iter( mProject->GetTracks() );
   Track *n;
   bool allTracks = true;

   // if at least one wave track is selected
   // apply only on the selected track
   for( n = iter.First(); n; n = iter.Next() )
      if( n->GetKind() == Track::Wave && n->GetSelected() )
      {
         allTracks = false;
         break;
      }

   //Apply action on wavetracks starting from
   //labeled regions in the end. This is to correctly perform
   //actions like 'Delete' which collapse the track area.
   n = iter.First();
   while (n)
   {
      if ((n->GetKind() == Track::Wave) &&
            (allTracks || n->GetSelected() || (bSyncLockedTracks && n->IsSyncLockSelected())))
      {
         WaveTrack *wt = ( WaveTrack* )n;
         for (int i = (int)regions.size() - 1; i >= 0; i--) {
            const Region &region = regions.at(i);
            (wt->*action)(region.start, region.end);
         }
      }
      n = iter.Next();
   }
}


//Executes the edit function on all selected wave tracks with
//regions specified by selected labels
//If No tracks selected, function is applied on all tracks
//Functions copy the edited regions to clipboard, possibly in multiple tracks
//This probably should not be called if *action() changes the timeline, because
// the copy needs to happen by track, and the timeline change by group.
void EditMenuCommands::EditClipboardByLabel( EditDestFunction action )
{
   Regions regions;

   GetRegionsByLabel( regions );
   if( regions.size() == 0 )
      return;

   TrackListIterator iter( mProject->GetTracks() );
   Track *n;
   bool allTracks = true;

   // if at least one wave track is selected
   // apply only on the selected track
   for( n = iter.First(); n; n = iter.Next() )
      if( n->GetKind() == Track::Wave && n->GetSelected() )
      {
         allTracks = false;
         break;
      }

   ClearClipboard();
   //Apply action on wavetracks starting from
   //labeled regions in the end. This is to correctly perform
   //actions like 'Cut' which collapse the track area.
   for( n = iter.First(); n; n = iter.Next() )
   {
      if( n->GetKind() == Track::Wave && ( allTracks || n->GetSelected() ) )
      {
         WaveTrack *wt = ( WaveTrack* )n;
         Track::Holder merged;
         for( int i = (int)regions.size() - 1; i >= 0; i-- )
         {
            const Region &region = regions.at(i);
            auto dest = ( wt->*action )( region.start, region.end );
            if( dest )
            {
               dest->SetChannel( wt->GetChannel() );
               dest->SetLinked( wt->GetLinked() );
               dest->SetName( wt->GetName() );
               if( !merged )
                  merged = std::move(dest);
               else
               {
                  // Paste to the beginning; unless this is the first region,
                  // offset the track to account for time between the regions
                  if (i < (int)regions.size() - 1)
                     merged->Offset(
                        regions.at(i + 1).start - region.end);

                  bool bResult = merged->Paste( 0.0 , dest.get() );
                  wxASSERT(bResult); // TO DO: Actually handle this.
                  wxUnusedVar(bResult);
               }
            }
            else  // nothing copied but there is a 'region', so the 'region' must be a 'point label' so offset
               if (i < (int)regions.size() - 1)
                  if (merged)
                     merged->Offset(
                        regions.at(i + 1).start - region.end);
         }
         if( merged )
            AudacityProject::msClipboard->Add( std::move(merged) );
      }
   }

   AudacityProject::msClipT0 = regions.front().start;
   AudacityProject::msClipT1 = regions.back().end;

   auto history = mProject->GetHistoryWindow();
   if (history)
      history->UpdateDisplay();
}

void EditMenuCommands::ClearClipboard()
{
   AudacityProject::msClipT0 = 0.0;
   AudacityProject::msClipT1 = 0.0;
   AudacityProject::msClipProject = NULL;
   if (AudacityProject::msClipboard) {
      AudacityProject::msClipboard->Clear();
   }
}

//get regions selected by selected labels
//removes unnecessary regions, overlapping regions are merged
//regions memory need to be deleted by the caller
void EditMenuCommands::GetRegionsByLabel( Regions &regions )
{
   const auto &viewInfo = mProject->GetViewInfo();
   TrackListIterator iter( mProject->GetTracks() );
   Track *n;

   //determine labelled regions
   for( n = iter.First(); n; n = iter.Next() )
      if( n->GetKind() == Track::Label && n->GetSelected() )
      {
         LabelTrack *lt = ( LabelTrack* )n;
         for( int i = 0; i < lt->GetNumLabels(); i++ )
         {
            const LabelStruct *ls = lt->GetLabel( i );
            if( ls->selectedRegion.t0() >= viewInfo.selectedRegion.t0() &&
                ls->selectedRegion.t1() <= viewInfo.selectedRegion.t1() )
               regions.push_back(Region(ls->getT0(), ls->getT1()));
         }
      }

   //anything to do ?
   if( regions.size() == 0 )
      return;

   //sort and remove unnecessary regions
   std::sort(regions.begin(), regions.end());
   unsigned int selected = 1;
   while( selected < regions.size() )
   {
      const Region &cur = regions.at( selected );
      Region &last = regions.at( selected - 1 );
      if( cur.start < last.end )
      {
         if( cur.end > last.end )
            last.end = cur.end;
         regions.erase( regions.begin() + selected );
      }
      else
         selected++;
   }
}

void EditMenuCommands::OnSelectAll()
{
   TrackList *const trackList = mProject->GetTracks();
   TrackListIterator iter(trackList);

   Track *t = iter.First();
   while (t) {
      t->SetSelected(true);
      t = iter.Next();
   }
   ViewInfo &viewInfo = mProject->GetViewInfo();
   viewInfo.selectedRegion.setTimes(
      trackList->GetMinOffset(), trackList->GetEndTime());

   mProject->ModifyState(false);

   mProject->GetTrackPanel()->Refresh(false);
   MixerBoard *const mixerBoard = mProject->GetMixerBoard();
   if (mixerBoard)
      mixerBoard->Refresh(false);
}

void EditMenuCommands::SelectAllIfNone()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   auto flags = mProject->GetUpdateFlags();
   if (!(flags & TracksSelectedFlag) ||
      (viewInfo.selectedRegion.isPoint()))
      OnSelectAll();
}

void EditMenuCommands::OnSelectNone()
{
   mProject->SelectNone();
   ViewInfo &viewInfo = mProject->GetViewInfo();
   viewInfo.selectedRegion.collapseToT0();
   mProject->ModifyState(false);
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
void EditMenuCommands::OnToggleSpectralSelection()
{
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   trackPanel->ToggleSpectralSelection();
   trackPanel->Refresh(false);
   mProject->ModifyState(false);
}

void EditMenuCommands::OnNextHigherPeakFrequency()
{
   DoNextPeakFrequency(true);
}


void EditMenuCommands::OnNextLowerPeakFrequency()
{
   DoNextPeakFrequency(false);
}

void EditMenuCommands::DoNextPeakFrequency(bool up)
{
   // Find the first selected wave track that is in a spectrogram view.
   WaveTrack *pTrack = 0;
   SelectedTrackListOfKindIterator iter(Track::Wave, mProject->GetTracks());
   for (Track *t = iter.First(); t; t = iter.Next()) {
      WaveTrack *const wt = static_cast<WaveTrack*>(t);
      const int display = wt->GetDisplay();
      if (display == WaveTrack::Spectrum) {
         pTrack = wt;
         break;
      }
   }

   if (pTrack) {
      TrackPanel *const trackPanel = mProject->GetTrackPanel();
      trackPanel->SnapCenterOnce(pTrack, up);
      trackPanel->Refresh(false);
      mProject->ModifyState(false);
   }
}
#endif

//this pops up a dialog which allows the left selection to be set.
//If playing/recording is happening, it sets the left selection at
//the current play position.
void EditMenuCommands::OnSetLeftSelection()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   bool bSelChanged = false;
   if ((mProject->GetAudioIOToken() > 0) &&
       gAudioIO->IsStreamActive(mProject->GetAudioIOToken()))
   {
      double indicator = gAudioIO->GetStreamTime();
      viewInfo.selectedRegion.setT0(indicator, false);
      bSelChanged = true;
   }
   else
   {
      wxString fmt = mProject->GetSelectionFormat();
      TimeDialog dlg(mProject, _("Set Left Selection Boundary"),
         fmt, mProject->GetRate(), viewInfo.selectedRegion.t0(), _("Position"));

      if (wxID_OK == dlg.ShowModal())
      {
         //Get the value from the dialog
         viewInfo.selectedRegion.setT0(
            std::max(0.0, dlg.GetTimeValue()), false);
         bSelChanged = true;
      }
   }

   if (bSelChanged)
   {
      mProject->ModifyState(false);
      mProject->GetTrackPanel()->Refresh(false);
   }
}

void EditMenuCommands::OnSetRightSelection()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   bool bSelChanged = false;
   if ((mProject->GetAudioIOToken() > 0) &&
       gAudioIO->IsStreamActive(mProject->GetAudioIOToken()))
   {
      double indicator = gAudioIO->GetStreamTime();
      viewInfo.selectedRegion.setT1(indicator, false);
      bSelChanged = true;
   }
   else
   {
      wxString fmt = mProject->GetSelectionFormat();
      TimeDialog dlg(mProject, _("Set Right Selection Boundary"),
         fmt, mProject->GetRate(), viewInfo.selectedRegion.t1(), _("Position"));

      if (wxID_OK == dlg.ShowModal())
      {
         //Get the value from the dialog
         viewInfo.selectedRegion.setT1(
            std::max(0.0, dlg.GetTimeValue()), false);
         bSelChanged = true;
      }
   }

   if (bSelChanged)
   {
      mProject->ModifyState(false);
      mProject->GetTrackPanel()->Refresh(false);
   }
}

void EditMenuCommands::OnSelectStartCursor()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   double minOffset = 1000000.0;

   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         if (t->GetOffset() < minOffset)
            minOffset = t->GetOffset();
      }

      t = iter.Next();
   }

   viewInfo.selectedRegion.setT0(minOffset);

   mProject->ModifyState(false);

   mProject->GetTrackPanel()->Refresh(false);
}

void EditMenuCommands::OnSelectCursorEnd()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   double maxEndOffset = -1000000.0;

   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         if (t->GetEndTime() > maxEndOffset)
            maxEndOffset = t->GetEndTime();
      }

      t = iter.Next();
   }

   viewInfo.selectedRegion.setT1(maxEndOffset);

   mProject->ModifyState(false);

   mProject->GetTrackPanel()->Refresh(false);
}

void EditMenuCommands::OnSelectCursorStoredCursor()
{
   auto &selectedRegion = mProject->GetViewInfo().selectedRegion;
   if (mProject->CursorPositionHasBeenStored()) {
      double cursorPositionCurrent = mProject->IsAudioActive()
         ? gAudioIO->GetStreamTime() : selectedRegion.t0();
      selectedRegion.setTimes(
         std::min(cursorPositionCurrent, mProject->CursorPositionStored()),
         std::max(cursorPositionCurrent, mProject->CursorPositionStored())
      );

      mProject->ModifyState(false);
      mProject->GetTrackPanel()->Refresh(false);
   }
}

void EditMenuCommands::OnSelectAllTracks()
{
   TrackListIterator iter(mProject->GetTracks());
   for (Track *t = iter.First(); t; t = iter.Next()) {
      t->SetSelected(true);
   }

   mProject->ModifyState(false);

   mProject->GetTrackPanel()->Refresh(false);
   MixerBoard *const mixerBoard = mProject->GetMixerBoard();
   if (mixerBoard)
      mixerBoard->Refresh(false);
}

void EditMenuCommands::OnSelectSyncLockSel()
{
   bool selected = false;
   TrackListIterator iter(mProject->GetTracks());
   for (Track *t = iter.First(); t; t = iter.Next())
   {
      if (t->IsSyncLockSelected()) {
         t->SetSelected(true);
         selected = true;
      }
   }

   if (selected)
      mProject->ModifyState(false);

   mProject->GetTrackPanel()->Refresh(false);
   MixerBoard *const mixerBoard = mProject->GetMixerBoard();
   if (mixerBoard)
      mixerBoard->Refresh(false);
}

void EditMenuCommands::OnZeroCrossing()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   const double t0 = NearestZeroCrossing(viewInfo.selectedRegion.t0());
   if (viewInfo.selectedRegion.isPoint())
      viewInfo.selectedRegion.setTimes(t0, t0);
   else {
      const double t1 = NearestZeroCrossing(viewInfo.selectedRegion.t1());
      viewInfo.selectedRegion.setTimes(t0, t1);
   }

   mProject->ModifyState(false);

   mProject->GetTrackPanel()->Refresh(false);
}

double EditMenuCommands::NearestZeroCrossing(double t0)
{
   // Window is 1/100th of a second.
   const double rate = mProject->GetRate();
   int windowSize = (int)(rate / 100);
   float *dist = new float[windowSize];
   int i, j;

   for (i = 0; i<windowSize; i++)
      dist[i] = 0.0;

   TrackListIterator iter(mProject->GetTracks());
   Track *track = iter.First();
   while (track) {
      if (!track->GetSelected() || track->GetKind() != (Track::Wave)) {
         track = iter.Next();
         continue;
      }
      WaveTrack *one = (WaveTrack *)track;
      int oneWindowSize = (int)(one->GetRate() / 100);
      float *oneDist = new float[oneWindowSize];
      auto s = one->TimeToLongSamples(t0);
      // fillTwo to ensure that missing values are treated as 2, and hence do not
      // get used as zero crossings.
      one->Get((samplePtr)oneDist, floatSample,
         s - oneWindowSize / 2, oneWindowSize, fillTwo);

      // Start by penalizing downward motion.  We prefer upward
      // zero crossings.
      if (oneDist[1] - oneDist[0] < 0)
         oneDist[0] = oneDist[0] * 6 + (oneDist[0] > 0 ? 0.3 : -0.3);
      for (i = 1; i<oneWindowSize; i++)
         if (oneDist[i] - oneDist[i - 1] < 0)
            oneDist[i] = oneDist[i] * 6 + (oneDist[i] > 0 ? 0.3 : -0.3);

      // Taking the absolute value -- apply a tiny LPF so square waves work.
      float newVal, oldVal = oneDist[0];
      oneDist[0] = fabs(.75 * oneDist[0] + .25 * oneDist[1]);
      for (i = 1; i<oneWindowSize - 1; i++)
      {
         newVal = fabs(.25 * oldVal + .5 * oneDist[i] + .25 * oneDist[i + 1]);
         oldVal = oneDist[i];
         oneDist[i] = newVal;
      }
      oneDist[oneWindowSize - 1] = fabs(.25 * oldVal +
         .75 * oneDist[oneWindowSize - 1]);

      // TODO: The mixed rate zero crossing code is broken,
      // if oneWindowSize > windowSize we'll miss out some
      // samples - so they will still be zero, so we'll use them.
      for (i = 0; i<windowSize; i++) {
         if (windowSize != oneWindowSize)
            j = i * (oneWindowSize - 1) / (windowSize - 1);
         else
            j = i;

         dist[i] += oneDist[j];
         // Apply a small penalty for distance from the original endpoint
         dist[i] += 0.1 * (abs(i - windowSize / 2)) / float(windowSize / 2);
      }

      delete[] oneDist;
      track = iter.Next();
   }

   // Find minimum
   int argmin = 0;
   float min = 3.0;
   for (i = 0; i<windowSize; i++) {
      if (dist[i] < min) {
         argmin = i;
         min = dist[i];
      }
   }

   delete[] dist;

   return t0 + (argmin - windowSize / 2) / rate;
}

void EditMenuCommands::OnCursorSelStart()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   TrackPanel *const trackPanel =  mProject->GetTrackPanel();
   viewInfo.selectedRegion.collapseToT0();
   mProject->ModifyState(false);
   trackPanel->ScrollIntoView(viewInfo.selectedRegion.t0());
   trackPanel->Refresh(false);
}

void EditMenuCommands::OnCursorSelEnd()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   viewInfo.selectedRegion.collapseToT1();
   mProject->ModifyState(false);
   trackPanel->ScrollIntoView(viewInfo.selectedRegion.t1());
   trackPanel->Refresh(false);
}

void EditMenuCommands::OnCursorTrackStart()
{
   double minOffset = 1000000.0;

   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         if (t->GetOffset() < minOffset)
            minOffset = t->GetOffset();
      }

      t = iter.Next();
   }

   if (minOffset < 0.0) minOffset = 0.0;
   ViewInfo &viewInfo = mProject->GetViewInfo();
   viewInfo.selectedRegion.setTimes(minOffset, minOffset);
   mProject->ModifyState(false);
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   trackPanel->ScrollIntoView(viewInfo.selectedRegion.t0());
   trackPanel->Refresh(false);
}

void EditMenuCommands::OnCursorTrackEnd()
{
   double maxEndOffset = -1000000.0;
   double thisEndOffset = 0.0;

   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         thisEndOffset = t->GetEndTime();
         if (thisEndOffset > maxEndOffset)
            maxEndOffset = thisEndOffset;
      }

      t = iter.Next();
   }

   ViewInfo &viewInfo = mProject->GetViewInfo();
   viewInfo.selectedRegion.setTimes(maxEndOffset, maxEndOffset);
   mProject->ModifyState(false);
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   trackPanel->ScrollIntoView(viewInfo.selectedRegion.t1());
   trackPanel->Refresh(false);
}
