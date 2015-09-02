#include "CursorAndFocusCommands.h"

#include <algorithm>

#include "../AudioIO.h"
#include "../MixerBoard.h"
#include "../Project.h"
#include "../TimeDialog.h"
#include "../TrackPanel.h"
#include "../WaveTrack.h"
#include "../commands/CommandManager.h"
#include "../toolbars/ToolManager.h"

#define FN(X) new ObjectCommandFunctor<CursorAndFocusCommands>(this, &CursorAndFocusCommands:: X )

CursorAndFocusCommands::CursorAndFocusCommands(AudacityProject *project)
   : mProject(project)
   , mRegionSave()
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
   mProject->OnCursorLeft(false, false);
}
