#include "CursorAndFocusCommands.h"

#include <algorithm>

#include "../AudioIO.h"
#include "../MixerBoard.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../TimeDialog.h"
#include "../TrackPanel.h"
#include "../TrackPanelAx.h"
#include "../WaveTrack.h"
#include "../commands/CommandManager.h"
#include "../toolbars/ToolManager.h"

#define FN(X) new ObjectCommandFunctor<CursorAndFocusCommands>(this, &CursorAndFocusCommands:: X )

CursorAndFocusCommands::CursorAndFocusCommands(AudacityProject *project)
   : mProject(project)
   , mRegionSave()
   , mLastSelectionAdjustment(wxGetLocalTimeMillis())
{
}

void CursorAndFocusCommands::Create(CommandManager *c)
{
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


   /////////////////////////////////////////////////////////////////////////////

   c->AddSeparator();

   c->AddItem(wxT("SelSave"), _("Re&gion Save"), FN(OnSelectionSave),
      WaveTracksSelectedFlag,
      WaveTracksSelectedFlag);
   c->AddItem(wxT("SelRestore"), _("Regio&n Restore"), FN(OnSelectionRestore),
      TracksExistFlag,
      TracksExistFlag);


   c->AddSeparator();
}

void CursorAndFocusCommands::CreateNonMenuCommands(CommandManager *c)
{
   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddGlobalCommand(wxT("PrevWindow"), _("Move backward thru active windows"), FN(PrevWindow), wxT("Alt+Shift+F6"));
   c->AddGlobalCommand(wxT("NextWindow"), _("Move forward thru active windows"), FN(NextWindow), wxT("Alt+F6"));

   c->AddCommand(wxT("PrevFrame"), _("Move backward from toolbars to tracks"), FN(PrevFrame), wxT("Ctrl+Shift+F6"));
   c->AddCommand(wxT("NextFrame"), _("Move forward from toolbars to tracks"), FN(NextFrame), wxT("Ctrl+F6"));

   c->AddCommand(wxT("SelStart"), _("Selection to Start"), FN(OnSelToStart), wxT("Shift+Home"));
   c->AddCommand(wxT("SelEnd"), _("Selection to End"), FN(OnSelToEnd), wxT("Shift+End"));

   c->SetDefaultFlags(AudioIOBusyFlag, AudioIOBusyFlag);
   c->AddCommand(wxT("SeekLeftShort"), _("Short seek left during playback"), FN(OnSeekLeftShort), wxT("Left\tallowDup"));
   c->AddCommand(wxT("SeekRightShort"), _("Short seek right during playback"), FN(OnSeekRightShort), wxT("Right\tallowDup"));
   c->AddCommand(wxT("SeekLeftLong"), _("Long seek left during playback"), FN(OnSeekLeftLong), wxT("Shift+Left\tallowDup"));
   c->AddCommand(wxT("SeekRightLong"), _("Long Seek right during playback"), FN(OnSeekRightLong), wxT("Shift+Right\tallowDup"));

   c->SetDefaultFlags(TracksExistFlag | TrackPanelHasFocus,
      TracksExistFlag | TrackPanelHasFocus);

   c->AddCommand(wxT("PrevTrack"), _("Move Focus to Previous Track"), FN(OnCursorUp), wxT("Up"));
   c->AddCommand(wxT("NextTrack"), _("Move Focus to Next Track"), FN(OnCursorDown), wxT("Down"));
   c->AddCommand(wxT("FirstTrack"), _("Move Focus to First Track"), FN(OnFirstTrack), wxT("Ctrl+Home"));
   c->AddCommand(wxT("LastTrack"), _("Move Focus to Last Track"), FN(OnLastTrack), wxT("Ctrl+End"));
   c->AddCommand(wxT("ShiftUp"), _("Move Focus to Previous and Select"), FN(OnShiftUp), wxT("Shift+Up"));
   c->AddCommand(wxT("ShiftDown"), _("Move Focus to Next and Select"), FN(OnShiftDown), wxT("Shift+Down"));
   c->AddCommand(wxT("Toggle"), _("Toggle Focused Track"), FN(OnToggle), wxT("Return"));
   c->AddCommand(wxT("ToggleAlt"), _("Toggle Focused Track"), FN(OnToggle), wxT("NUMPAD_ENTER"));
   c->AddCommand(wxT("CursorLeft"), _("Cursor Left"), FN(OnCursorLeft), wxT("Left\twantKeyup\tallowDup"));
   c->AddCommand(wxT("CursorRight"), _("Cursor Right"), FN(OnCursorRight), wxT("Right\twantKeyup\tallowDup"));
   c->AddCommand(wxT("CursorShortJumpLeft"), _("Cursor Short Jump Left"), FN(OnCursorShortJumpLeft), wxT(","));
   c->AddCommand(wxT("CursorShortJumpRight"), _("Cursor Short Jump Right"), FN(OnCursorShortJumpRight), wxT("."));
   c->AddCommand(wxT("CursorLongJumpLeft"), _("Cursor Long Jump Left"), FN(OnCursorLongJumpLeft), wxT("Shift+,"));
   c->AddCommand(wxT("CursorLongJumpRight"), _("Cursor Long Jump Right"), FN(OnCursorLongJumpRight), wxT("Shift+."));
   c->AddCommand(wxT("SelExtLeft"), _("Selection Extend Left"), FN(OnSelExtendLeft), wxT("Shift+Left\twantKeyup\tallowDup"));
   c->AddCommand(wxT("SelExtRight"), _("Selection Extend Right"), FN(OnSelExtendRight), wxT("Shift+Right\twantKeyup\tallowDup"));
   c->AddCommand(wxT("SelSetExtLeft"), _("Set (or Extend) Left Selection"), FN(OnSelSetExtendLeft));
   c->AddCommand(wxT("SelSetExtRight"), _("Set (or Extend) Right Selection"), FN(OnSelSetExtendRight));
   c->AddCommand(wxT("SelCntrLeft"), _("Selection Contract Left"), FN(OnSelContractLeft), wxT("Ctrl+Shift+Right\twantKeyup"));
   c->AddCommand(wxT("SelCntrRight"), _("Selection Contract Right"), FN(OnSelContractRight), wxT("Ctrl+Shift+Left\twantKeyup"));
}

void CursorAndFocusCommands::OnSelectAll()
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

void CursorAndFocusCommands::SelectAllIfNone()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   wxUint32 flags = mProject->GetUpdateFlags();
   if (((flags & TracksSelectedFlag) == 0) ||
      (viewInfo.selectedRegion.isPoint()))
      OnSelectAll();
}

void CursorAndFocusCommands::OnSelectNone()
{
   mProject->SelectNone();
   ViewInfo &viewInfo = mProject->GetViewInfo();
   viewInfo.selectedRegion.collapseToT0();
   mProject->ModifyState(false);
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
void CursorAndFocusCommands::OnToggleSpectralSelection()
{
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   trackPanel->ToggleSpectralSelection();
   trackPanel->Refresh(false);
   mProject->ModifyState(false);
}

void CursorAndFocusCommands::OnNextHigherPeakFrequency()
{
   DoNextPeakFrequency(true);
}


void CursorAndFocusCommands::OnNextLowerPeakFrequency()
{
   DoNextPeakFrequency(false);
}

void CursorAndFocusCommands::DoNextPeakFrequency(bool up)
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
void CursorAndFocusCommands::OnSetLeftSelection()
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

void CursorAndFocusCommands::OnSetRightSelection()
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

void CursorAndFocusCommands::OnSelectStartCursor()
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

void CursorAndFocusCommands::OnSelectCursorEnd()
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

void CursorAndFocusCommands::OnSelectAllTracks()
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

void CursorAndFocusCommands::OnSelectSyncLockSel()
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

void CursorAndFocusCommands::OnZeroCrossing()
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

double CursorAndFocusCommands::NearestZeroCrossing(double t0)
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
      sampleCount s = one->TimeToLongSamples(t0);
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

void CursorAndFocusCommands::OnCursorSelStart()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   viewInfo.selectedRegion.collapseToT0();
   mProject->ModifyState(false);
   trackPanel->ScrollIntoView(viewInfo.selectedRegion.t0());
   trackPanel->Refresh(false);
}

void CursorAndFocusCommands::OnCursorSelEnd()
{
   ViewInfo &viewInfo = mProject->GetViewInfo();
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   viewInfo.selectedRegion.collapseToT1();
   mProject->ModifyState(false);
   trackPanel->ScrollIntoView(viewInfo.selectedRegion.t1());
   trackPanel->Refresh(false);
}

void CursorAndFocusCommands::OnCursorTrackStart()
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

void CursorAndFocusCommands::OnCursorTrackEnd()
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

void CursorAndFocusCommands::OnSelectionSave()
{
   mRegionSave = mProject->GetViewInfo().selectedRegion;
}

void CursorAndFocusCommands::OnSelectionRestore()
{
   if ((mRegionSave.t0() == 0.0) &&
      (mRegionSave.t1() == 0.0))
      return;

   mProject->GetViewInfo().selectedRegion = mRegionSave;

   mProject->ModifyState(false);

   mProject->GetTrackPanel()->Refresh(false);
}

void CursorAndFocusCommands::PrevWindow()
{
   wxWindow *w = wxGetTopLevelParent(wxWindow::FindFocus());
   const wxWindowList & list = mProject->GetChildren();
   wxWindowList::compatibility_iterator iter;

   // If the project window has the current focus, start the search with the last child
   if (w == mProject)
   {
      iter = list.GetLast();
   }
   // Otherwise start the search with the current window's previous sibling
   else
   {
      if (list.Find(w))
         iter = list.Find(w)->GetPrevious();
   }

   // Search for the previous toplevel window
   while (iter)
   {
      // If it's a toplevel and is visible (we have come hidden windows), then we're done
      w = iter->GetData();
      if (w->IsTopLevel() && w->IsShown())
      {
         break;
      }

      // Find the window in this projects children.  If the window with the
      // focus isn't a child of this project (like when a dialog is created
      // without specifying a parent), then we'll get back NULL here.
      iter = list.Find(w);
      if (iter)
      {
         iter = iter->GetPrevious();
      }
   }

   // Ran out of siblings, so make the current project active
   if (!iter && mProject->IsEnabled())
   {
      w = mProject;
   }

   // And make sure it's on top (only for floating windows...project window will not raise)
   // (Really only works on Windows)
   w->Raise();
}

void CursorAndFocusCommands::NextWindow()
{
   wxWindow *w = wxGetTopLevelParent(wxWindow::FindFocus());
   const wxWindowList & list = mProject->GetChildren();
   wxWindowList::compatibility_iterator iter;

   // If the project window has the current focus, start the search with the first child
   if (w == mProject)
   {
      iter = list.GetFirst();
   }
   // Otherwise start the search with the current window's next sibling
   else
   {
      // Find the window in this projects children.  If the window with the
      // focus isn't a child of this project (like when a dialog is created
      // without specifying a parent), then we'll get back NULL here.
      iter = list.Find(w);
      if (iter)
      {
         iter = iter->GetNext();
      }
   }

   // Search for the next toplevel window
   while (iter)
   {
      // If it's a toplevel, visible (we have hidden windows) and is enabled,
      // then we're done.  The IsEnabled() prevents us from moving away from 
      // a modal dialog because all other toplevel windows will be disabled.
      w = iter->GetData();
      if (w->IsTopLevel() && w->IsShown() && w->IsEnabled())
      {
         break;
      }

      // Get the next sibling
      iter = iter->GetNext();
   }

   // Ran out of siblings, so make the current project active
   if (!iter && mProject->IsEnabled())
   {
      w = mProject;
   }

   // And make sure it's on top (only for floating windows...project window will not raise)
   // (Really only works on Windows)
   w->Raise();
}

void CursorAndFocusCommands::PrevFrame()
{
   switch (mProject->GetFocusedFrame())
   {
   case TopDockHasFocus:
      mProject->mToolManager->GetBotDock()->SetFocus();
      break;

   case TrackPanelHasFocus:
      mProject->mToolManager->GetTopDock()->SetFocus();
      break;

   case BotDockHasFocus:
      mProject->GetTrackPanel()->SetFocus();
      break;
   }
}

void CursorAndFocusCommands::NextFrame()
{
   switch (mProject->GetFocusedFrame())
   {
   case TopDockHasFocus:
      mProject->GetTrackPanel()->SetFocus();
      break;

   case TrackPanelHasFocus:
      mProject->mToolManager->GetBotDock()->SetFocus();
      break;

   case BotDockHasFocus:
      mProject->mToolManager->GetTopDock()->SetFocus();
      break;
   }
}

void CursorAndFocusCommands::OnSelToStart()
{
   mProject->Rewind(true);
   mProject->ModifyState(false);
}

void CursorAndFocusCommands::OnSelToEnd()
{
   mProject->SkipEnd(true);
   mProject->ModifyState(false);
}

void CursorAndFocusCommands::OnSeekLeftShort()
{
   OnCursorLeft(false, false);
}

void CursorAndFocusCommands::OnSeekRightShort()
{
   OnCursorRight(false, false);
}

void CursorAndFocusCommands::OnSeekLeftLong()
{
   OnCursorLeft(true, false);
}

void CursorAndFocusCommands::OnSeekRightLong()
{
   OnCursorRight(true, false);
}

void CursorAndFocusCommands::OnCursorUp()
{
   OnPrevTrack(false);
}

void CursorAndFocusCommands::OnCursorDown()
{
   OnNextTrack(false);
}

void CursorAndFocusCommands::OnFirstTrack()
{
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   Track *const t = trackPanel->GetFocusedTrack();
   if (!t)
      return;

   TrackListIterator iter(mProject->GetTracks());
   Track *f = iter.First();
   if (t != f)
   {
      trackPanel->SetFocusedTrack(f);
      mProject->ModifyState(false);
   }
   trackPanel->EnsureVisible(f);
}

void CursorAndFocusCommands::OnLastTrack()
{
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   Track *const t = trackPanel->GetFocusedTrack();
   if (!t)
      return;

   TrackListIterator iter(mProject->GetTracks());
   Track *l = iter.Last();
   if (t != l)
   {
      trackPanel->SetFocusedTrack(l);
      mProject->ModifyState(false);
   }
   trackPanel->EnsureVisible(l);
}

void CursorAndFocusCommands::OnShiftUp()
{
   OnPrevTrack(true);
}

void CursorAndFocusCommands::OnShiftDown()
{
   OnNextTrack(true);
}

/// The following method moves to the previous track
/// selecting and unselecting depending if you are on the start of a
/// block or not.

/// \todo Merge related methods, OnPrevTrack and
/// OnNextTrack.
void CursorAndFocusCommands::OnPrevTrack(bool shift)
{
   bool circularTrackNavigation;
   gPrefs->Read(wxT("/GUI/CircularTrackNavigation"), &circularTrackNavigation,
      false);
   TrackList *const trackList = mProject->GetTracks();
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   TrackListIterator iter(trackList);
   Track *t = trackPanel->GetFocusedTrack();
   if (t == NULL)   // if there isn't one, focus on last
   {
      t = iter.Last();
      trackPanel->SetFocusedTrack(t);
      trackPanel->EnsureVisible(t);
      mProject->ModifyState(false);
      return;
   }

   Track* p = NULL;
   bool tSelected = false;
   bool pSelected = false;
   if (shift)
   {
      p = trackList->GetPrev(t, true); // Get previous track
      if (p == NULL)   // On first track
      {
         // JKC: wxBell() is probably for accessibility, so a blind
         // user knows they were at the top track.
         wxBell();
         if (circularTrackNavigation)
         {
            TrackListIterator iter(trackList);
            p = iter.Last();
         }
         else
         {
            trackPanel->EnsureVisible(t);
            return;
         }
      }
      tSelected = t->GetSelected();
      if (p)
         pSelected = p->GetSelected();
      if (tSelected && pSelected)
      {
         trackList->Select(t, false);
         trackPanel->SetFocusedTrack(p);   // move focus to next track down
         trackPanel->EnsureVisible(p);
         mProject->ModifyState(false);
         return;
      }
      if (tSelected && !pSelected)
      {
         trackList->Select(p, true);
         trackPanel->SetFocusedTrack(p);   // move focus to next track down
         trackPanel->EnsureVisible(p);
         mProject->ModifyState(false);
         return;
      }
      if (!tSelected && pSelected)
      {
         trackList->Select(p, false);
         trackPanel->SetFocusedTrack(p);   // move focus to next track down
         trackPanel->EnsureVisible(p);
         mProject->ModifyState(false);
         return;
      }
      if (!tSelected && !pSelected)
      {
         trackList->Select(t, true);
         trackPanel->SetFocusedTrack(p);   // move focus to next track down
         trackPanel->EnsureVisible(p);
         mProject->ModifyState(false);
         return;
      }
   }
   else
   {
      p = trackList->GetPrev(t, true); // Get next track
      if (p == NULL)   // On last track so stay there?
      {
         wxBell();
         if (circularTrackNavigation)
         {
            TrackListIterator iter(trackList);
            for (Track *d = iter.First(); d; d = iter.Next(true))
            {
               p = d;
            }
            trackPanel->SetFocusedTrack(p);   // Wrap to the first track
            trackPanel->EnsureVisible(p);
            mProject->ModifyState(false);
            return;
         }
         else
         {
            trackPanel->EnsureVisible(t);
            return;
         }
      }
      else
      {
         trackPanel->SetFocusedTrack(p);   // move focus to next track down
         trackPanel->EnsureVisible(p);
         mProject->ModifyState(false);
         return;
      }
   }
}

/// The following method moves to the next track,
/// selecting and unselecting depending if you are on the start of a
/// block or not.
void CursorAndFocusCommands::OnNextTrack(bool shift)
{
   bool circularTrackNavigation;
   gPrefs->Read(wxT("/GUI/CircularTrackNavigation"), &circularTrackNavigation,
      false);
   TrackList *const trackList = mProject->GetTracks();
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   Track *t;
   Track *n;
   TrackListIterator iter(trackList);
   bool tSelected, nSelected;

   t = trackPanel->GetFocusedTrack();   // Get currently focused track
   if (t == NULL)   // if there isn't one, focus on first
   {
      t = iter.First();
      trackPanel->SetFocusedTrack(t);
      trackPanel->EnsureVisible(t);
      mProject->ModifyState(false);
      return;
   }

   if (shift)
   {
      n = trackList->GetNext(t, true); // Get next track
      if (n == NULL)   // On last track so stay there
      {
         wxBell();
         if (circularTrackNavigation)
         {
            TrackListIterator iter(trackList);
            n = iter.First();
         }
         else
         {
            trackPanel->EnsureVisible(t);
            return;
         }
      }
      tSelected = t->GetSelected();
      nSelected = n->GetSelected();
      if (tSelected && nSelected)
      {
         trackList->Select(t, false);
         trackPanel->SetFocusedTrack(n);   // move focus to next track down
         trackPanel->EnsureVisible(n);
         mProject->ModifyState(false);
         return;
      }
      if (tSelected && !nSelected)
      {
         trackList->Select(n, true);
         trackPanel->SetFocusedTrack(n);   // move focus to next track down
         trackPanel->EnsureVisible(n);
         mProject->ModifyState(false);
         return;
      }
      if (!tSelected && nSelected)
      {
         trackList->Select(n, false);
         trackPanel->SetFocusedTrack(n);   // move focus to next track down
         trackPanel->EnsureVisible(n);
         mProject->ModifyState(false);
         return;
      }
      if (!tSelected && !nSelected)
      {
         trackList->Select(t, true);
         trackPanel->SetFocusedTrack(n);   // move focus to next track down
         trackPanel->EnsureVisible(n);
         mProject->ModifyState(false);
         return;
      }
   }
   else
   {
      n = trackList->GetNext(t, true); // Get next track
      if (n == NULL)   // On last track so stay there
      {
         wxBell();
         if (circularTrackNavigation)
         {
            TrackListIterator iter(trackList);
            n = iter.First();
            trackPanel->SetFocusedTrack(n);   // Wrap to the first track
            trackPanel->EnsureVisible(n);
            mProject->ModifyState(false);
            return;
         }
         else
         {
            trackPanel->EnsureVisible(t);
            return;
         }
      }
      else
      {
         trackPanel->SetFocusedTrack(n);   // move focus to next track down
         trackPanel->EnsureVisible(n);
         mProject->ModifyState(false);
         return;
      }
   }
}

void CursorAndFocusCommands::OnToggle()
{
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   Track *t;

   t = trackPanel->GetFocusedTrack();   // Get currently focused track
   if (!t)
      return;

   mProject->GetTracks()->Select(t, !t->GetSelected());
   trackPanel->EnsureVisible(t);
   mProject->ModifyState(false);

   trackPanel->GetAx().Updated();

   return;
}

void CursorAndFocusCommands::OnCursorLeft(const wxEvent * evt)
{
   OnCursorLeft(false, false, evt->GetEventType() == wxEVT_KEY_UP);
}

void CursorAndFocusCommands::OnCursorRight(const wxEvent * evt)
{
   OnCursorRight(false, false, evt->GetEventType() == wxEVT_KEY_UP);
}

void CursorAndFocusCommands::OnCursorShortJumpLeft()
{
   OnCursorMove(false, true, false);
}

void CursorAndFocusCommands::OnCursorShortJumpRight()
{
   OnCursorMove(true, true, false);
}

void CursorAndFocusCommands::OnCursorLongJumpLeft()
{
   OnCursorMove(false, true, true);
}

void CursorAndFocusCommands::OnCursorLongJumpRight()
{
   OnCursorMove(true, true, true);
}

// Move the cursor forward or backward, while paused or while playing.
// forward=true: Move cursor forward; forward=false: Move cursor backwards
// jump=false: Move cursor determined by zoom; jump=true: Use seek times
// longjump=false: Use mSeekShort; longjump=true: Use mSeekLong
void CursorAndFocusCommands::OnCursorMove(bool forward, bool jump, bool longjump)
{
   double seekShort, seekLong;
   gPrefs->Read(wxT("/AudioIO/SeekShortPeriod"), &seekShort, 1.0);
   gPrefs->Read(wxT("/AudioIO/SeekLongPeriod"), &seekLong, 15.0);

   // PRL:  nobody calls this yet with !jump

   double positiveSeekStep;
   bool byPixels;
   if (jump) {
      if (!longjump) {
         positiveSeekStep = seekShort;
      }
      else {
         positiveSeekStep = seekLong;
      }
      byPixels = false;
   }
   else {
      positiveSeekStep = 1.0;
      byPixels = true;
   }
   bool mayAccelerate = !jump;
   SeekLeftOrRight
      (!forward, false, false, false,
      0, mayAccelerate, mayAccelerate,
      positiveSeekStep, byPixels,
      positiveSeekStep, byPixels);

   mProject->ModifyState(false);
}

void CursorAndFocusCommands::OnSelExtendLeft(const wxEvent * evt)
{
   OnCursorLeft(true, false, evt->GetEventType() == wxEVT_KEY_UP);
}

void CursorAndFocusCommands::OnSelExtendRight(const wxEvent * evt)
{
   OnCursorRight(true, false, evt->GetEventType() == wxEVT_KEY_UP);
}

void CursorAndFocusCommands::OnSelSetExtendLeft()
{
   OnBoundaryMove(true, false);
}

void CursorAndFocusCommands::OnSelSetExtendRight()
{
   OnBoundaryMove(false, false);
}

void CursorAndFocusCommands::OnBoundaryMove(bool left, bool boundaryContract)
{
   // Move the left/right selection boundary, to either expand or contract the selection
   // left=true: operate on left boundary; left=false: operate on right boundary
   // boundaryContract=true: contract region; boundaryContract=false: expand region.

   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   ViewInfo &viewInfo = mProject->GetViewInfo();

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   wxLongLong curtime = ::wxGetLocalTimeMillis();
   int pixels = 1;
   if (curtime - mLastSelectionAdjustment < 50)
   {
      pixels = 4;
   }
   mLastSelectionAdjustment = curtime;

   if (mProject->IsAudioActive())
   {
      double indicator = gAudioIO->GetStreamTime();
      if (left)
         viewInfo.selectedRegion.setT0(indicator, false);
      else
         viewInfo.selectedRegion.setT1(indicator);

      mProject->ModifyState(false);
      trackPanel->Refresh(false);
   }
   else
   {
      // BOUNDARY MOVEMENT
      // Contract selection from the right to the left
      if (boundaryContract)
      {
         if (left) {
            // Reduce and constrain left boundary (counter-intuitive)
            // Move the left boundary by at most the desired number of pixels,
            // but not past the right
            viewInfo.selectedRegion.setT0(
               std::min(viewInfo.selectedRegion.t1(),
                        viewInfo.OffsetTimeByPixels(
                        viewInfo.selectedRegion.t0(),
                        pixels
            )));

            // Make sure it's visible
            trackPanel->ScrollIntoView(viewInfo.selectedRegion.t0());
         }
         else
         {
            // Reduce and constrain right boundary (counter-intuitive)
            // Move the right boundary by at most the desired number of pixels,
            // but not past the left
            viewInfo.selectedRegion.setT1(
               std::max(viewInfo.selectedRegion.t0(),
                        viewInfo.OffsetTimeByPixels(
                        viewInfo.selectedRegion.t1(),
                        -pixels
            )));

            // Make sure it's visible
            trackPanel->ScrollIntoView(viewInfo.selectedRegion.t1());
         }
      }
      // BOUNDARY MOVEMENT
      // Extend selection toward the left
      else
      {
         if (left) {
            // Expand and constrain left boundary
            viewInfo.selectedRegion.setT0(
               std::max(0.0,
                        viewInfo.OffsetTimeByPixels(
                        viewInfo.selectedRegion.t0(),
                        -pixels
            )));

            // Make sure it's visible
            trackPanel->ScrollIntoView(viewInfo.selectedRegion.t0());
         }
         else
         {
            // Expand and constrain right boundary
            const double end = mProject->GetTracks()->GetEndTime();
            viewInfo.selectedRegion.setT1(
               std::min(end,
                        viewInfo.OffsetTimeByPixels(
                        viewInfo.selectedRegion.t1(),
                        pixels
            )));

            // Make sure it's visible
            trackPanel->ScrollIntoView(viewInfo.selectedRegion.t1());
         }
      }
      trackPanel->Refresh(false);
      mProject->ModifyState(false);
   }
}

void CursorAndFocusCommands::OnSelContractLeft(const wxEvent * evt)
{
   OnCursorRight(true, true, evt->GetEventType() == wxEVT_KEY_UP);
}

void CursorAndFocusCommands::OnCursorRight(bool shift, bool ctrl, bool keyup)
{
   double seekShort, seekLong;
   gPrefs->Read(wxT("/AudioIO/SeekShortPeriod"), &seekShort, 1.0);
   gPrefs->Read(wxT("/AudioIO/SeekLongPeriod"), &seekLong, 15.0);

   // PRL:  What I found and preserved, strange though it be:
   // During playback:  jump depends on preferences and is independent of the zoom
   // and does not vary if the key is held
   // Else: jump depends on the zoom and gets bigger if the key is held
   int snapToTime = mProject->GetSnapTo();
   double quietSeekStepPositive = 1.0; // pixels
   double audioSeekStepPositive = shift ? seekLong : seekShort;
   SeekLeftOrRight
      (false, shift, ctrl, keyup, snapToTime, true, false,
      quietSeekStepPositive, true,
      audioSeekStepPositive, false);
}

void CursorAndFocusCommands::OnSelContractRight(const wxEvent * evt)
{
   OnCursorLeft(true, true, evt->GetEventType() == wxEVT_KEY_UP);
}

void CursorAndFocusCommands::OnCursorLeft(bool shift, bool ctrl, bool keyup)
{
   double seekShort, seekLong;
   gPrefs->Read(wxT("/AudioIO/SeekShortPeriod"), &seekShort, 1.0);
   gPrefs->Read(wxT("/AudioIO/SeekLongPeriod"), &seekLong, 15.0);

   // PRL:  What I found and preserved, strange though it be:
   // During playback:  jump depends on preferences and is independent of the zoom
   // and does not vary if the key is held
   // Else: jump depends on the zoom and gets bigger if the key is held
   int snapToTime = mProject->GetSnapTo();
   double quietSeekStepPositive = 1.0; // pixels
   double audioSeekStepPositive = shift ? seekLong : seekShort;
   SeekLeftOrRight
      (true, shift, ctrl, keyup, snapToTime, true, false,
      quietSeekStepPositive, true,
      audioSeekStepPositive, false);
}

// Handle small cursor and play head movements
void CursorAndFocusCommands::SeekLeftOrRight
(bool leftward, bool shift, bool ctrl, bool keyup,
 int snapToTime, bool mayAccelerateQuiet, bool mayAccelerateAudio,
 double quietSeekStepPositive, bool quietStepIsPixels,
 double audioSeekStepPositive, bool audioStepIsPixels)
{
   if (keyup)
   {
      if (mProject->IsAudioActive())
      {
         return;
      }

      mProject->ModifyState(false);
      return;
   }

   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   ViewInfo &viewInfo = mProject->GetViewInfo();

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   const wxLongLong curtime = ::wxGetLocalTimeMillis();
   enum { MIN_INTERVAL = 50 };
   const bool fast = (curtime - mLastSelectionAdjustment < MIN_INTERVAL);

   // How much faster should the cursor move if shift is down?
   enum { LARGER_MULTIPLIER = 4 };
   int multiplier = (fast && mayAccelerateQuiet) ? LARGER_MULTIPLIER : 1;
   if (leftward)
      multiplier = -multiplier;

   if (shift && ctrl)
   {
      mLastSelectionAdjustment = curtime;

      // Contract selection
      // Reduce and constrain (counter-intuitive)
      if (leftward) {
         const double t1 = viewInfo.selectedRegion.t1();
         viewInfo.selectedRegion.setT1(
            std::max(viewInfo.selectedRegion.t0(),
               snapToTime
               ? GridMove(t1, multiplier)
               : quietStepIsPixels
                  ? viewInfo.OffsetTimeByPixels(
                        t1, int(multiplier * quietSeekStepPositive))
                  : t1 +  multiplier * quietSeekStepPositive
         ));

         // Make sure it's visible.
         trackPanel->ScrollIntoView(viewInfo.selectedRegion.t1());
      }
      else {
         const double t0 = viewInfo.selectedRegion.t0();
         viewInfo.selectedRegion.setT0(
            std::min(viewInfo.selectedRegion.t1(),
               snapToTime
               ? GridMove(t0, multiplier)
               : quietStepIsPixels
                  ? viewInfo.OffsetTimeByPixels(
                     t0, int(multiplier * quietSeekStepPositive))
                  : t0 + multiplier * quietSeekStepPositive
         ));

         // Make sure new position is in view.
         trackPanel->ScrollIntoView(viewInfo.selectedRegion.t0());
      }
      trackPanel->Refresh(false);
   }
   else if (mProject->IsAudioActive()) {
#ifdef EXPERIMENTAL_IMPROVED_SEEKING
      if (gAudioIO->GetLastPlaybackTime() < mLastSelectionAdjustment) {
         // Allow time for the last seek to output a buffer before
         // discarding samples again
         // Do not advance mLastSelectionAdjustment
         return;
      }
#endif
      mLastSelectionAdjustment = curtime;

      // Ignore the multiplier for the quiet case
      multiplier = (fast && mayAccelerateAudio) ? LARGER_MULTIPLIER : 1;
      if (leftward)
         multiplier = -multiplier;

      // If playing, reposition
      double seconds;
      if (audioStepIsPixels) {
         const double streamTime = gAudioIO->GetStreamTime();
         const double newTime =
            viewInfo.OffsetTimeByPixels(streamTime, int(audioSeekStepPositive));
         seconds = newTime - streamTime;
      }
      else
         seconds = multiplier * audioSeekStepPositive;
      gAudioIO->SeekStream(seconds);
      return;
   }
   else if (shift)
   {
      mLastSelectionAdjustment = curtime;

      // Extend selection
      // Expand and constrain
      if (leftward) {
         const double t0 = viewInfo.selectedRegion.t0();
         viewInfo.selectedRegion.setT0(
            std::max(0.0,
               snapToTime
               ? GridMove(t0, multiplier)
               : quietStepIsPixels
                  ? viewInfo.OffsetTimeByPixels(
                        t0, int(multiplier * quietSeekStepPositive))
                  : t0 + multiplier * quietSeekStepPositive
         ));

         // Make sure it's visible.
         trackPanel->ScrollIntoView(viewInfo.selectedRegion.t0());
      }
      else {
         const double end = mProject->GetTracks()->GetEndTime();
         const double t1 = viewInfo.selectedRegion.t1();
         viewInfo.selectedRegion.setT1(
            std::min(end,
               snapToTime
               ? GridMove(t1, multiplier)
               : quietStepIsPixels
                  ? viewInfo.OffsetTimeByPixels(
                        t1, int(multiplier * quietSeekStepPositive))
                  : t1 + multiplier * quietSeekStepPositive
         ));

         // Make sure new position is in view.
         trackPanel->ScrollIntoView(viewInfo.selectedRegion.t1());
      }
      trackPanel->Refresh(false);
   }
   else
   {
      mLastSelectionAdjustment = curtime;

      // Move the cursor
      // Already in cursor mode?
      if (viewInfo.selectedRegion.isPoint())
      {
         // Move and constrain
         const double end = mProject->GetTracks()->GetEndTime();
         const double t0 = viewInfo.selectedRegion.t0();
         viewInfo.selectedRegion.setT0(
            std::max(0.0,
               std::min(end,
                  snapToTime
                  ? GridMove(t0, multiplier)
                  : quietStepIsPixels
                     ? viewInfo.OffsetTimeByPixels(
                          t0, int(multiplier * quietSeekStepPositive))
                     : t0 + multiplier * quietSeekStepPositive)),
            false // do not swap selection boundaries
         );
         viewInfo.selectedRegion.collapseToT0();

         // Move the visual cursor, avoiding an unnecessary complete redraw
         trackPanel->DrawOverlays(false);
      }
      else
      {
         // Transition to cursor mode.
         if (leftward)
            viewInfo.selectedRegion.collapseToT0();
         else
            viewInfo.selectedRegion.collapseToT1();
         trackPanel->Refresh(false);
      }

      // Make sure new position is in view
      trackPanel->ScrollIntoView(viewInfo.selectedRegion.t1());
   }
}

// Handles moving a selection edge with the keyboard in snap-to-time mode;
// returns the moved value.
// Will move at least minPix pixels -- set minPix positive to move forward,
// negative to move backward.
double CursorAndFocusCommands::GridMove(double t, int minPix)
{
   NumericConverter nc
      (NumericConverter::TIME, mProject->GetSelectionFormat(), t, mProject->GetRate());

   const ViewInfo &viewInfo = mProject->GetViewInfo();

   // Try incrementing/decrementing the value; if we've moved far enough we're
   // done
   double result;
   minPix >= 0 ? nc.Increment() : nc.Decrement();
   result = nc.GetValue();
   if (std::abs(viewInfo.TimeToPosition(result) - viewInfo.TimeToPosition(t))
      >= abs(minPix))
      return result;

   // Otherwise, move minPix pixels, then snap to the time.
   result = viewInfo.OffsetTimeByPixels(t, minPix);
   nc.SetValue(result);
   result = nc.GetValue();
   return result;
}
