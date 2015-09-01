#include "../Experimental.h"
#include "TracksMenuCommands.h"

#include <wx/combobox.h>

#include "../AudioIO.h"
#include "../LabelDialog.h"
#include "../LabelTrack.h"
#include "../Mix.h"
#include "../MixerBoard.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../ShuttleGui.h"
#include "../TimeTrack.h"
#include "../TrackPanel.h"
#include "../WaveTrack.h"
#include "../commands/CommandManager.h"

#define FN(X) new ObjectCommandFunctor<TracksMenuCommands>(this, &TracksMenuCommands:: X )

TracksMenuCommands::TracksMenuCommands(AudacityProject *project)
   : mProject(project)
   , mAlignLabelsCount(0)
{
}

void TracksMenuCommands::Create(CommandManager *c)
{
   c->BeginMenu(_("&Tracks"));
   {
      c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

      //////////////////////////////////////////////////////////////////////////

      c->BeginSubMenu(_("Add &New"));
      {
         c->AddItem(wxT("NewMonoTrack"), _("&Mono Track"), FN(OnNewWaveTrack), wxT("Ctrl+Shift+N"));
         c->AddItem(wxT("NewStereoTrack"), _("&Stereo Track"), FN(OnNewStereoTrack));
         c->AddItem(wxT("NewLabelTrack"), _("&Label Track"), FN(OnNewLabelTrack));
         c->AddItem(wxT("NewTimeTrack"), _("&Time Track"), FN(OnNewTimeTrack));
      }
      c->EndSubMenu();

      //////////////////////////////////////////////////////////////////////////

      c->AddSeparator();

      c->AddItem(wxT("Stereo to Mono"), _("Stereo Trac&k to Mono"), FN(OnStereoToMono),
         AudioIONotBusyFlag | StereoRequiredFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | StereoRequiredFlag | WaveTracksSelectedFlag);
      c->AddItem(wxT("MixAndRender"), _("Mi&x and Render"), FN(OnMixAndRender),
         AudioIONotBusyFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | WaveTracksSelectedFlag);
      c->AddItem(wxT("MixAndRenderToNewTrack"), _("Mix and Render to Ne&w Track"), FN(OnMixAndRenderToNewTrack), wxT("Ctrl+Shift+M"),
         AudioIONotBusyFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | WaveTracksSelectedFlag);
      c->AddItem(wxT("Resample"), _("&Resample..."), FN(OnResample),
         AudioIONotBusyFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | WaveTracksSelectedFlag);

      c->AddSeparator();

      c->AddItem(wxT("RemoveTracks"), _("Remo&ve Tracks"), FN(OnRemoveTracks),
         AudioIONotBusyFlag | TracksSelectedFlag,
         AudioIONotBusyFlag | TracksSelectedFlag);

      c->AddSeparator();

      c->AddItem(wxT("MuteAllTracks"), _("&Mute All Tracks"), FN(OnMuteAllTracks), wxT("Ctrl+U"));
      c->AddItem(wxT("UnMuteAllTracks"), _("&Unmute All Tracks"), FN(OnUnMuteAllTracks), wxT("Ctrl+Shift+U"));

      c->AddSeparator();

      wxArrayString alignLabelsNoSync;
      alignLabelsNoSync.Add(_("&Align End to End"));
      alignLabelsNoSync.Add(_("Align &Together"));

      wxArrayString alignLabels;
      alignLabels.Add(_("Start to &Zero"));
      alignLabels.Add(_("Start to &Cursor/Selection Start"));
      alignLabels.Add(_("Start to Selection &End"));
      alignLabels.Add(_("End to Cu&rsor/Selection Start"));
      alignLabels.Add(_("End to Selection En&d"));
      mAlignLabelsCount = alignLabels.GetCount();

      // Calling c->SetCommandFlags() after AddItemList for "Align" and "AlignMove"
      // does not correctly set flags for submenus, so do it this way.
      c->SetDefaultFlags(AudioIONotBusyFlag | TracksSelectedFlag,
         AudioIONotBusyFlag | TracksSelectedFlag);

      c->BeginSubMenu(_("&Align Tracks"));
      {
         c->AddItemList(wxT("Align"), alignLabelsNoSync, FN(OnAlignNoSync));
         c->AddSeparator();
         c->AddItemList(wxT("Align"), alignLabels, FN(OnAlign));
      }
      c->EndSubMenu();

      //////////////////////////////////////////////////////////////////////////

      // TODO: Can these labels be made clearer? Do we need this sub-menu at all?
      c->BeginSubMenu(_("Move Sele&ction when Aligning"));
      {
         c->AddItemList(wxT("AlignMove"), alignLabels, FN(OnAlignMoveSel));
         c->SetCommandFlags(wxT("AlignMove"),
            AudioIONotBusyFlag | TracksSelectedFlag,
            AudioIONotBusyFlag | TracksSelectedFlag);
      }
      c->EndSubMenu();

      c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

      //////////////////////////////////////////////////////////////////////////

#ifdef EXPERIMENTAL_SCOREALIGN
#error
      c->AddItem(wxT("ScoreAlign"), _("Synchronize MIDI with Audio"), FN(OnScoreAlign),
         AudioIONotBusyFlag | NoteTracksSelectedFlag | WaveTracksSelectedFlag,
         AudioIONotBusyFlag | NoteTracksSelectedFlag | WaveTracksSelectedFlag);
#endif // EXPERIMENTAL_SCOREALIGN

#ifdef EXPERIMENTAL_SYNC_LOCK
      c->AddSeparator();

      c->AddCheck(wxT("SyncLock"), _("Sync-&Lock Tracks"), FN(OnSyncLock), 0,
         AlwaysEnabledFlag, AlwaysEnabledFlag);
#endif

      c->AddSeparator();

      c->AddItem(wxT("AddLabel"), _("Add Label At &Selection"), FN(OnAddLabel), wxT("Ctrl+B"),
         AlwaysEnabledFlag, AlwaysEnabledFlag);
      c->AddItem(wxT("AddLabelPlaying"), _("Add Label At &Playback Position"),
         FN(OnAddLabelPlaying),
#ifdef __WXMAC__
         wxT("Ctrl+."),
#else
         wxT("Ctrl+M"),
#endif
         0, AudioIONotBusyFlag);

      c->AddItem(wxT("EditLabels"), _("&Edit Labels..."), FN(OnEditLabels));

      c->AddSeparator();

      //////////////////////////////////////////////////////////////////////////

      c->BeginSubMenu(_("S&ort Tracks"));
      {
         c->AddItem(wxT("SortByTime"), _("by &Start time"), FN(OnSortTime),
            TracksExistFlag,
            TracksExistFlag);
         c->AddItem(wxT("SortByName"), _("by &Name"), FN(OnSortName),
            TracksExistFlag,
            TracksExistFlag);
      }
      c->EndSubMenu();
   }
   c->EndMenu();
}

void TracksMenuCommands::CreateNonMenuCommands(CommandManager *c)
{
   c->SetDefaultFlags(TracksExistFlag | TrackPanelHasFocus,
      TracksExistFlag | TrackPanelHasFocus);

   c->AddCommand(wxT("TrackPan"), _("Change pan on focused track"), FN(OnTrackPan), wxT("Shift+P"));
   c->AddCommand(wxT("TrackPanLeft"), _("Pan left on focused track"), FN(OnTrackPanLeft), wxT("Alt+Shift+Left"));
}

void TracksMenuCommands::OnNewWaveTrack()
{
   WaveTrack *const t =
      mProject->GetTrackFactory()->NewWaveTrack
         (mProject->GetDefaultFormat(), mProject->GetRate());
   mProject->SelectNone();

   mProject->GetTracks()->Add(t);
   t->SetSelected(true);

   mProject->PushState(_("Created new audio track"), _("New Track"));

   mProject->RedrawProject();
   mProject->GetTrackPanel()->EnsureVisible(t);
}

void TracksMenuCommands::OnNewStereoTrack()
{
   WaveTrack *t =
      mProject->GetTrackFactory()->NewWaveTrack
         (mProject->GetDefaultFormat(), mProject->GetRate());
   t->SetChannel(Track::LeftChannel);
   mProject->SelectNone();

   mProject->GetTracks()->Add(t);
   t->SetSelected(true);
   t->SetLinked(true);

   t = mProject->GetTrackFactory()->NewWaveTrack
          (mProject->GetDefaultFormat(), mProject->GetRate());
   t->SetChannel(Track::RightChannel);

   mProject->GetTracks()->Add(t);
   t->SetSelected(true);

   mProject->PushState(_("Created new stereo audio track"), _("New Track"));

   mProject->RedrawProject();
   mProject->GetTrackPanel()->EnsureVisible(t);
}

void TracksMenuCommands::OnNewLabelTrack()
{
   LabelTrack *const t = new LabelTrack(mProject->GetDirManager());

   mProject->SelectNone();

   mProject->GetTracks()->Add(t);
   t->SetSelected(true);

   mProject->PushState(_("Created new label track"), _("New Track"));

   mProject->RedrawProject();
   mProject->GetTrackPanel()->EnsureVisible(t);
}

void TracksMenuCommands::OnNewTimeTrack()
{
   if (mProject->GetTracks()->GetTimeTrack()) {
      wxMessageBox(_("This version of Audacity only allows one time track for each project window."));
      return;
   }

   TimeTrack *t = new TimeTrack(mProject->GetDirManager());

   mProject->SelectNone();

   mProject->GetTracks()->AddToHead(t);
   t->SetSelected(true);

   mProject->PushState(_("Created new time track"), _("New Track"));

   mProject->RedrawProject();
   mProject->GetTrackPanel()->EnsureVisible(t);
}

void TracksMenuCommands::OnStereoToMono(int WXUNUSED(index))
{
   mProject->OnEffect(EffectManager::Get().GetEffectByIdentifier(wxT("StereoToMono")),
      OnEffectFlagsConfigured);
}

void TracksMenuCommands::OnMixAndRender()
{
   HandleMixAndRender(false);
}

void TracksMenuCommands::OnMixAndRenderToNewTrack()
{
   HandleMixAndRender(true);
}

void TracksMenuCommands::HandleMixAndRender(bool toNewTrack)
{
   WaveTrack *newLeft = NULL;
   WaveTrack *newRight = NULL;

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);

   if (::MixAndRender(mProject->GetTracks(), mProject->GetTrackFactory(),
      mProject->GetRate(), mProject->GetDefaultFormat(), 0.0, 0.0,
      &newLeft, &newRight)) {

      // Remove originals, get stats on what tracks were mixed

      TrackListIterator iter(mProject->GetTracks());
      Track *t = iter.First();
      int selectedCount = 0;
      wxString firstName;

      while (t) {
         if (t->GetSelected() && (t->GetKind() == Track::Wave)) {
            if (selectedCount == 0)
               firstName = t->GetName();

            // Add one to the count if it's an unlinked track, or if it's the first
            // in a stereo pair
            if (t->GetLinked() || !t->GetLink())
               selectedCount++;

            if (!toNewTrack) {
               t = iter.RemoveCurrent(true);
            }
            else {
               t = iter.Next();
            };
         }
         else
            t = iter.Next();
      }

      // Add new tracks

      mProject->GetTracks()->Add(newLeft);
      if (newRight)
         mProject->GetTracks()->Add(newRight);

      // If we're just rendering (not mixing), keep the track name the same
      if (selectedCount == 1) {
         newLeft->SetName(firstName);
         if (newRight)
            newRight->SetName(firstName);
      }

      // Smart history/undo message
      if (selectedCount == 1) {
         wxString msg;
         msg.Printf(_("Rendered all audio in track '%s'"), firstName.c_str());
         /* i18n-hint: Convert the audio into a more usable form, so apply
         * panning and amplification and write to some external file.*/
         mProject->PushState(msg, _("Render"));
      }
      else {
         wxString msg;
         if (newRight)
            msg.Printf(_("Mixed and rendered %d tracks into one new stereo track"),
            selectedCount);
         else
            msg.Printf(_("Mixed and rendered %d tracks into one new mono track"),
            selectedCount);
         mProject->PushState(msg, _("Mix and Render"));
      }

      mProject->GetTrackPanel()->SetFocus();
      mProject->GetTrackPanel()->SetFocusedTrack(newLeft);
      mProject->GetTrackPanel()->EnsureVisible(newLeft);
      mProject->RedrawProject();
   }
}

void TracksMenuCommands::OnResample()
{
   TrackListIterator iter(mProject->GetTracks());

   int newRate;

   while (true)
   {
      wxDialog dlg(mProject, wxID_ANY, wxString(_("Resample")));
      dlg.SetName(dlg.GetTitle());
      ShuttleGui S(&dlg, eIsCreating);
      wxString rate;
      wxArrayString rates;
      wxComboBox *cb;

      rate.Printf(wxT("%ld"), lrint(mProject->GetRate()));

      rates.Add(wxT("8000"));
      rates.Add(wxT("11025"));
      rates.Add(wxT("16000"));
      rates.Add(wxT("22050"));
      rates.Add(wxT("32000"));
      rates.Add(wxT("44100"));
      rates.Add(wxT("48000"));
      rates.Add(wxT("88200"));
      rates.Add(wxT("96000"));
      rates.Add(wxT("176400"));
      rates.Add(wxT("192000"));
      rates.Add(wxT("352800"));
      rates.Add(wxT("384000"));

      S.StartVerticalLay(true);
      {
         S.AddSpace(-1, 15);

         S.StartHorizontalLay(wxCENTER, false);
         {
            cb = S.AddCombo(_("New sample rate (Hz):"),
               rate,
               &rates);
         }
         S.EndHorizontalLay();

         S.AddSpace(-1, 15);

         S.AddStandardButtons();
      }
      S.EndVerticalLay();

      dlg.Layout();
      dlg.Fit();
      dlg.Center();

      if (dlg.ShowModal() != wxID_OK)
      {
         return;  // user cancelled dialog
      }

      long lrate;
      if (cb->GetValue().ToLong(&lrate) && lrate >= 1 && lrate <= 1000000)
      {
         newRate = (int)lrate;
         break;
      }

      wxMessageBox(_("The entered value is invalid"), _("Error"),
         wxICON_ERROR, mProject);
   }

   int ndx = 0;
   for (Track *t = iter.First(); t; t = iter.Next())
   {
      wxString msg;

      msg.Printf(_("Resampling track %d"), ++ndx);

      ProgressDialog progress(_("Resample"), msg);

      if (t->GetSelected() && t->GetKind() == Track::Wave)
         if (!((WaveTrack*)t)->Resample(newRate, &progress))
            break;
   }

   mProject->PushState(_("Resampled audio track(s)"), _("Resample Track"));
   mProject->RedrawProject();

   // Need to reset
   mProject->FinishAutoScroll();
}

void TracksMenuCommands::OnRemoveTracks()
{
   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();
   Track *f = NULL;
   Track *l = NULL;

   while (t) {
      if (t->GetSelected()) {
         if (mProject->GetMixerBoard() && (t->GetKind() == Track::Wave))
            mProject->GetMixerBoard()->RemoveTrackCluster((WaveTrack*)t);
         if (!f)
            f = l;         // Capture the track preceeding the first removed track
         t = iter.RemoveCurrent(true);
      }
      else {
         l = t;
         t = iter.Next();
      }
   }

   // All tracks but the last were removed...try to use the last track
   if (!f)
      f = l;

   // Try to use the first track after the removal or, if none,
   // the track preceeding the removal
   if (f) {
      t = mProject->GetTracks()->GetNext(f, true);
      if (t)
         f = t;
   }

   // If we actually have something left, then make sure it's seen
   if (f)
      mProject->GetTrackPanel()->EnsureVisible(f);

   mProject->PushState(_("Removed audio track(s)"), _("Remove Track"));

   mProject->GetTrackPanel()->UpdateViewIfNoTracks();
   mProject->GetTrackPanel()->Refresh(false);

   if (mProject->GetMixerBoard())
      mProject->GetMixerBoard()->Refresh(true);
}

void TracksMenuCommands::OnMuteAllTracks()
{
   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();

   while (t)
   {
      if (t->GetKind() == Track::Wave)
         t->SetMute(true);

      t = iter.Next();
   }

   mProject->ModifyState(true);
   mProject->RedrawProject();
   if (mProject->GetMixerBoard())
      mProject->GetMixerBoard()->UpdateMute();
}

void TracksMenuCommands::OnUnMuteAllTracks()
{
   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();

   while (t)
   {
      t->SetMute(false);
      t = iter.Next();
   }

   mProject->ModifyState(true);
   mProject->RedrawProject();
   if (mProject->GetMixerBoard())
      mProject->GetMixerBoard()->UpdateMute();
}

void TracksMenuCommands::OnAlignNoSync(int index)
{
   // Add length of alignLabels array so that we can handle this in AudacityProject::HandleAlign.
   HandleAlign(index + mAlignLabelsCount, false);
}

void TracksMenuCommands::OnAlign(int index)
{
   HandleAlign(index, false);
}

void TracksMenuCommands::OnAlignMoveSel(int index)
{
   HandleAlign(index, true);
}

void TracksMenuCommands::HandleAlign(int index, bool moveSel)
{
   enum {
      kAlignStartZero = 0,
      kAlignStartSelStart,
      kAlignStartSelEnd,
      kAlignEndSelStart,
      kAlignEndSelEnd,
      // The next two are only in one subMenu, so more easily handled at the end.
      kAlignEndToEnd,
      kAlignTogether
   };

   TrackListIterator iter(mProject->GetTracks());
   wxString action;
   wxString shortAction;
   double offset;
   double minOffset = 1000000000.0;
   double maxEndOffset = 0.0;
   double leftOffset = 0.0;
   bool bRightChannel = false;
   double avgOffset = 0.0;
   int numSelected = 0;
   Track *t = iter.First();
   double delta = 0.0;
   double newPos = -1.0;
   wxArrayDouble trackStartArray;
   wxArrayDouble trackEndArray;
   double firstTrackOffset=0.0f;

   while (t) {
      // We only want Wave and Note tracks here.
#if defined(USE_MIDI)
      if (t->GetSelected() && ((t->GetKind() == Track::Wave) ||
                               (t->GetKind() == Track::Note))) {
#else
      if (t->GetSelected() && (t->GetKind() == Track::Wave)) {
#endif
         offset = t->GetOffset();
         if (t->GetLinked()) {   // Left channel of stereo track.
            leftOffset = offset;
            bRightChannel = true; // next track is the right channel.
         } else {
            if (bRightChannel) {
               // Align channel with earlier start  time
               offset = (offset < leftOffset)? offset : leftOffset;
               leftOffset = 0.0;
               bRightChannel = false;
            }
            avgOffset += offset;
            if (numSelected == 0) {
               firstTrackOffset = offset; // For Align End to End.
            }
            numSelected++;
         }
         trackStartArray.Add(t->GetStartTime());
         trackEndArray.Add(t->GetEndTime());

         if (offset < minOffset)
            minOffset = offset;
         if (t->GetEndTime() > maxEndOffset)
            maxEndOffset = t->GetEndTime();
      }
      t = iter.Next();
   }

   avgOffset /= numSelected;  // numSelected is mono/stereo tracks not channels.

   switch(index) {
   case kAlignStartZero:
      delta = -minOffset;
      action = _("start to zero");
      shortAction = _("Start");
      break;
   case kAlignStartSelStart:
      delta = mProject->GetViewInfo().selectedRegion.t0() - minOffset;
      action = _("start to cursor/selection start");
      shortAction = _("Start");
      break;
   case kAlignStartSelEnd:
      delta = mProject->GetViewInfo().selectedRegion.t1() - minOffset;
      action = _("start to selection end");
      shortAction = _("Start");
      break;
   case kAlignEndSelStart:
      delta = mProject->GetViewInfo().selectedRegion.t0() - maxEndOffset;
      action = _("end to cursor/selection start");
      shortAction = _("End");
      break;
   case kAlignEndSelEnd:
      delta = mProject->GetViewInfo().selectedRegion.t1() - maxEndOffset;
      action = _("end to selection end");
      shortAction = _("End");
      break;
   // index set in alignLabelsNoSync
   case kAlignEndToEnd:
      newPos = firstTrackOffset;
      action = _("end to end");
      shortAction = _("End to End");
      break;
   case kAlignTogether:
      newPos = avgOffset;
      action = _("together");
      shortAction = _("Together");
   }

   if ((unsigned)index >= mAlignLabelsCount) { // This is an alignLabelsNoSync command.
      TrackListIterator iter(mProject->GetTracks());
      Track *t = iter.First();
      double leftChannelStart = 0.0;
      double leftChannelEnd = 0.0;
      double rightChannelStart = 0.0;
      double rightChannelEnd = 0.0;
      int arrayIndex = 0;
      while (t) {
         // This shifts different tracks in different ways, so no sync-lock move.
         // Only align Wave and Note tracks end to end.
#if defined(USE_MIDI)
         if (t->GetSelected() && ((t->GetKind() == Track::Wave) ||
                                  (t->GetKind() == Track::Note))) {
#else
         if (t->GetSelected() && (t->GetKind() == Track::Wave)) {
#endif
            t->SetOffset(newPos);   // Move the track

            if (t->GetLinked()) {   // Left channel of stereo track.
               leftChannelStart = trackStartArray[arrayIndex];
               leftChannelEnd = trackEndArray[arrayIndex];
               rightChannelStart = trackStartArray[1+arrayIndex];
               rightChannelEnd = trackEndArray[1+arrayIndex];
               bRightChannel = true;   // next track is the right channel.
               // newPos is the offset for the earlier channel.
               // If right channel started first, offset the left channel.
               if (rightChannelStart < leftChannelStart) {
                  t->SetOffset(newPos + leftChannelStart - rightChannelStart);
               }
               arrayIndex++;
            } else {
               if (bRightChannel) {
                  // If left channel started first, offset the right channel.
                  if (leftChannelStart < rightChannelStart) {
                     t->SetOffset(newPos + rightChannelStart - leftChannelStart);
                  }
                  if (index == kAlignEndToEnd) {
                     // Now set position for start of next track.
                     newPos += wxMax(leftChannelEnd, rightChannelEnd) - wxMin(leftChannelStart, rightChannelStart);
                  }
                  bRightChannel = false;
               } else { // Mono track
                  if (index == kAlignEndToEnd) {
                     newPos += (trackEndArray[arrayIndex] - trackStartArray[arrayIndex]);
                  }
               }
               arrayIndex++;
            }
         }
         t = iter.Next();
      }
      if (index == kAlignEndToEnd) {
         mProject->OnZoomFit();
      }
   }

   if (delta != 0.0) {
      TrackListIterator iter(mProject->GetTracks());
      Track *t = iter.First();

      while (t) {
         // For a fixed-distance shift move sync-lock selected tracks also.
         if (t->GetSelected() || t->IsSyncLockSelected()) {
            t->SetOffset(t->GetOffset() + delta);
         }
         t = iter.Next();
      }
   }

   if (moveSel) {
      mProject->GetViewInfo().selectedRegion.move(delta);
      action = wxString::Format(_("Aligned/Moved %s"), action.c_str());
      shortAction = wxString::Format(_("Align %s/Move"),shortAction.c_str());
      mProject->PushState(action, shortAction);
   } else {
      action = wxString::Format(_("Aligned %s"), action.c_str());
      shortAction = wxString::Format(_("Align %s"),shortAction.c_str());
      mProject->PushState(action, shortAction);
   }

   mProject->RedrawProject();
}

#ifdef EXPERIMENTAL_SCOREALIGN
// rough relative amount of time to compute one
//    frame of audio or midi, or one cell of matrix, or one iteration
//    of smoothing, measured on a 1.9GHz Core 2 Duo in 32-bit mode
//    (see COLLECT_TIMING_DATA below)
#define AUDIO_WORK_UNIT 0.004F
#define MIDI_WORK_UNIT 0.0001F
#define MATRIX_WORK_UNIT 0.000002F
#define SMOOTHING_WORK_UNIT 0.000001F

// Write timing data to a file; useful for calibrating AUDIO_WORK_UNIT,
// MIDI_WORK_UNIT, MATRIX_WORK_UNIT, and SMOOTHING_WORK_UNIT coefficients
// Data is written to timing-data.txt; look in
//     audacity-src/win/Release/modules/
#define COLLECT_TIMING_DATA

// Audacity Score Align Progress class -- progress reports come here
class ASAProgress : public SAProgress {
 private:
   float mTotalWork;
   float mFrames[2];
   long mTotalCells; // how many matrix cells?
   long mCellCount; // how many cells so far?
   long mPrevCellCount; // cell_count last reported with Update()
   ProgressDialog *mProgress;
   #ifdef COLLECT_TIMING_DATA
      FILE *mTimeFile;
      wxDateTime mStartTime;
      long iterations;
   #endif

 public:
   ASAProgress() {
      smoothing = false;
      mProgress = NULL;
      #ifdef COLLECT_TIMING_DATA
         mTimeFile = fopen("timing-data.txt", "w");
      #endif
   }
   ~ASAProgress() {
      delete mProgress;
      #ifdef COLLECT_TIMING_DATA
         fclose(mTimeFile);
      #endif
   }
   virtual void set_phase(int i) {
      float work[2]; // chromagram computation work estimates
      float work2, work3 = 0; // matrix and smoothing work estimates
      SAProgress::set_phase(i);
      #ifdef COLLECT_TIMING_DATA
         long ms = 0;
         wxDateTime now = wxDateTime::UNow();
         fprintf(mTimeFile, "Phase %d begins at %s\n",
                 i, now.FormatTime().c_str());
         if (i != 0)
            ms = now.Subtract(mStartTime).GetMilliseconds().ToLong();
         mStartTime = now;
      #endif
      if (i == 0) {
         mCellCount = 0;
         for (int j = 0; j < 2; j++) {
            mFrames[j] = durations[j] / frame_period;
         }
         mTotalWork = 0;
         for (int j = 0; j < 2; j++) {
             work[j] =
                (is_audio[j] ? AUDIO_WORK_UNIT : MIDI_WORK_UNIT) * mFrames[j];
             mTotalWork += work[j];
         }
         mTotalCells = mFrames[0] * mFrames[1];
         work2 = mTotalCells * MATRIX_WORK_UNIT;
         mTotalWork += work2;
         // arbitarily assume 60 iterations to fit smooth segments and
         // per frame per iteration is SMOOTHING_WORK_UNIT
         if (smoothing) {
            work3 =
               wxMax(mFrames[0], mFrames[1]) * SMOOTHING_WORK_UNIT * 40;
            mTotalWork += work3;
         }
         #ifdef COLLECT_TIMING_DATA
            fprintf(mTimeFile, " mTotalWork (an estimate) = %g\n", mTotalWork);
            fprintf(mTimeFile, " work0 = %g, frames %g, is_audio %d\n",
                    work[0], mFrames[0], is_audio[0]);
            fprintf(mTimeFile, " work1 = %g, frames %g, is_audio %d\n",
                    work[1], mFrames[1], is_audio[1]);
            fprintf(mTimeFile, "work2 = %g, work3 = %g\n", work2, work3);
         #endif
         mProgress = new ProgressDialog(_("Synchronize MIDI with Audio"),
                               _("Synchronizing MIDI and Audio Tracks"));
      } else if (i < 3) {
         fprintf(mTimeFile,
               "Phase %d took %d ms for %g frames, coefficient = %g s/frame\n",
               i - 1, ms, mFrames[i - 1], (ms * 0.001) / mFrames[i - 1]);
      } else if (i == 3) {
        fprintf(mTimeFile,
                "Phase 2 took %d ms for %d cells, coefficient = %g s/cell\n",
                ms, mCellCount, (ms * 0.001) / mCellCount);
      } else if (i == 4) {
        fprintf(mTimeFile, "Phase 3 took %d ms for %d iterations on %g frames, coefficient = %g s per frame per iteration\n",
                ms, iterations, wxMax(mFrames[0], mFrames[1]),
                (ms * 0.001) / (wxMax(mFrames[0], mFrames[1]) * iterations));
      }
   }
   virtual bool set_feature_progress(float s) {
      float work;
      if (phase == 0) {
         float f = s / frame_period;
         work = (is_audio[0] ? AUDIO_WORK_UNIT : MIDI_WORK_UNIT) * f;
      } else if (phase == 1) {
         float f = s / frame_period;
         work = (is_audio[0] ? AUDIO_WORK_UNIT : MIDI_WORK_UNIT) * mFrames[0] +
                (is_audio[1] ? AUDIO_WORK_UNIT : MIDI_WORK_UNIT) * f;
      }
      int updateResult = mProgress->Update(int(work), int(mTotalWork));
      return (updateResult == eProgressSuccess);
   }
   virtual bool set_matrix_progress(int cells) {
      mCellCount += cells;
      float work =
             (is_audio[0] ? AUDIO_WORK_UNIT : MIDI_WORK_UNIT) * mFrames[0] +
             (is_audio[1] ? AUDIO_WORK_UNIT : MIDI_WORK_UNIT) * mFrames[1];
      work += mCellCount * MATRIX_WORK_UNIT;
      int updateResult = mProgress->Update(int(work), int(mTotalWork));
      return (updateResult == eProgressSuccess);
   }
   virtual bool set_smoothing_progress(int i) {
      iterations = i;
      float work =
             (is_audio[0] ? AUDIO_WORK_UNIT : MIDI_WORK_UNIT) * mFrames[0] +
             (is_audio[1] ? AUDIO_WORK_UNIT : MIDI_WORK_UNIT) * mFrames[1] +
             MATRIX_WORK_UNIT * mFrames[0] * mFrames[1];
      work += i * wxMax(mFrames[0], mFrames[1]) * SMOOTHING_WORK_UNIT;
      int updateResult = mProgress->Update(int(work), int(mTotalWork));
      return (updateResult == eProgressSuccess);
   }
};


long mixer_process(void *mixer, float **buffer, long n)
{
   Mixer *mix = (Mixer *) mixer;
   long frame_count = mix->Process(n);
   *buffer = (float *) mix->GetBuffer();
   return frame_count;
}

void AudacityProject::OnScoreAlign()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   int numWaveTracksSelected = 0;
   int numNoteTracksSelected = 0;
   int numOtherTracksSelected = 0;
   NoteTrack *nt;
   NoteTrack *alignedNoteTrack;
   double endTime = 0.0;

   // Iterate through once to make sure that there is exactly
   // one WaveTrack and one NoteTrack selected.
   while (t) {
      if (t->GetSelected()) {
         if (t->GetKind() == Track::Wave) {
            numWaveTracksSelected++;
            WaveTrack *wt = (WaveTrack *) t;
            endTime = endTime > wt->GetEndTime() ? endTime : wt->GetEndTime();
         } else if(t->GetKind() == Track::Note) {
            numNoteTracksSelected++;
            nt = (NoteTrack *) t;
         } else numOtherTracksSelected++;
      }
      t = iter.Next();
   }

   if(numWaveTracksSelected == 0 ||
      numNoteTracksSelected != 1 ||
      numOtherTracksSelected != 0){
      wxMessageBox(wxString::Format(wxT("Please select at least one audio track and one MIDI track.")));
      return;
   }

   // Creating the dialog also stores dialog into gScoreAlignDialog so
   // that it can be delted by CloseScoreAlignDialog() either here or
   // if the program is quit by the user while the dialog is up.
   ScoreAlignParams params;
   ScoreAlignDialog *dlog = new ScoreAlignDialog(NULL, params);
   CloseScoreAlignDialog();

   if (params.mStatus != wxID_OK) return;

   // We're going to do it.
   //pushing the state before the change is wrong (I think)
   //PushState(_("Sync MIDI with Audio"), _("Sync MIDI with Audio"));
   // Make a copy of the note track in case alignment is canceled or fails
   alignedNoteTrack = (NoteTrack *) nt->Duplicate();
   // Duplicate() on note tracks serializes seq to a buffer, but we need
   // the seq, so Duplicate again and discard the track with buffer. The
   // test is here in case Duplicate() is changed in the future.
   if (alignedNoteTrack->GetSequence() == NULL) {
      NoteTrack *temp = (NoteTrack *) alignedNoteTrack->Duplicate();
      delete alignedNoteTrack;
      alignedNoteTrack = temp;
      assert(alignedNoteTrack->GetSequence());
   }
   // Remove offset from NoteTrack because audio is
   // mixed starting at zero and incorporating clip offsets.
   if (alignedNoteTrack->GetOffset() < 0) {
      // remove the negative offset data before alignment
      nt->Clear(alignedNoteTrack->GetOffset(), 0);
   } else if (alignedNoteTrack->GetOffset() > 0) {
      alignedNoteTrack->Shift(alignedNoteTrack->GetOffset());
   }
   alignedNoteTrack->SetOffset(0);

   WaveTrack **waveTracks;
   mTracks->GetWaveTracks(true /* selectionOnly */,
                          &numWaveTracksSelected, &waveTracks);

   Mixer *mix = new Mixer(numWaveTracksSelected,   // int numInputTracks
                          waveTracks,              // WaveTrack **inputTracks
                          mTracks->GetTimeTrack(), // TimeTrack *timeTrack
                          0.0,                     // double startTime
                          endTime,                 // double stopTime
                          2,                       // int numOutChannels
                          44100,                   // int outBufferSize
                          true,                    // bool outInterleaved
                          mRate,                   // double outRate
                          floatSample,             // sampleFormat outFormat
                          true,                    // bool highQuality = true
                          NULL);                   // MixerSpec *mixerSpec = NULL
   delete [] waveTracks;

   ASAProgress *progress = new ASAProgress;

   // There's a lot of adjusting made to incorporate the note track offset into
   // the note track while preserving the position of notes within beats and
   // measures. For debugging, you can see just the pre-scorealign note track
   // manipulation by setting SKIP_ACTUAL_SCORE_ALIGNMENT. You could then, for
   // example, save the modified note track in ".gro" form to read the details.
   //#define SKIP_ACTUAL_SCORE_ALIGNMENT 1
#ifndef SKIP_ACTUAL_SCORE_ALIGNMENT
   int result = scorealign((void *) mix, &mixer_process,
                           2 /* channels */, 44100.0 /* srate */, endTime,
                           alignedNoteTrack->GetSequence(), progress, params);
#else
   int result = SA_SUCCESS;
#endif

   delete progress;
   delete mix;

   if (result == SA_SUCCESS) {
      mTracks->Replace(nt, alignedNoteTrack, true);
      RedrawProject();
      wxMessageBox(wxString::Format(
         _("Alignment completed: MIDI from %.2f to %.2f secs, Audio from %.2f to %.2f secs."),
         params.mMidiStart, params.mMidiEnd,
         params.mAudioStart, params.mAudioEnd));
      PushState(_("Sync MIDI with Audio"), _("Sync MIDI with Audio"));
   } else if (result == SA_TOOSHORT) {
      delete alignedNoteTrack;
      wxMessageBox(wxString::Format(
         _("Alignment error: input too short: MIDI from %.2f to %.2f secs, Audio from %.2f to %.2f secs."),
         params.mMidiStart, params.mMidiEnd,
         params.mAudioStart, params.mAudioEnd));
   } else if (result == SA_CANCEL) {
      // wrong way to recover...
      //GetActiveProject()->OnUndo(); // recover any changes to note track
      delete alignedNoteTrack;
      return; // no message when user cancels alignment
   } else {
      //GetActiveProject()->OnUndo(); // recover any changes to note track
      delete alignedNoteTrack;
      wxMessageBox(_("Internal error reported by alignment process."));
   }
}
#endif /* EXPERIMENTAL_SCOREALIGN */

void TracksMenuCommands::OnSyncLock()
{
   bool bSyncLockTracks;
   gPrefs->Read(wxT("/GUI/SyncLockTracks"), &bSyncLockTracks, false);
   gPrefs->Write(wxT("/GUI/SyncLockTracks"), !bSyncLockTracks);
   gPrefs->Flush();

   // Toolbar, project sync-lock handled within
   mProject->ModifyAllProjectToolbarMenus();

   mProject->GetTrackPanel()->Refresh(false);
}

void TracksMenuCommands::OnAddLabel()
{
   DoAddLabel(mProject->GetViewInfo().selectedRegion);
}

void TracksMenuCommands::OnAddLabelPlaying()
{
   if (mProject->GetAudioIOToken()>0 &&
      gAudioIO->IsStreamActive(mProject->GetAudioIOToken())) {
      double indicator = gAudioIO->GetStreamTime();
      DoAddLabel(SelectedRegion(indicator, indicator));
   }
}

int TracksMenuCommands::DoAddLabel(const SelectedRegion &region)
{
   LabelTrack *lt = NULL;

   // If the focused track is a label track, use that
   Track *t = mProject->GetTrackPanel()->GetFocusedTrack();
   if (t && t->GetKind() == Track::Label) {
      lt = (LabelTrack *)t;
   }

   // Otherwise look for a label track after the focused track
   if (!lt) {
      TrackListIterator iter(mProject->GetTracks());
      if (t)
         iter.StartWith(t);
      else
         t = iter.First();

      while (t && !lt) {
         if (t->GetKind() == Track::Label)
            lt = (LabelTrack *)t;

         t = iter.Next();
      }
   }

   // If none found, start a new label track and use it
   if (!lt) {
      lt = new LabelTrack(mProject->GetDirManager());
      mProject->GetTracks()->Add(lt);
   }

   // LLL: Commented as it seemed a little forceful to remove users
   //      selection when adding the label.  This does not happen if
   //      you select several tracks and the last one of those is a
   //      label track...typing a label will not clear the selections.
   //
   //   SelectNone();
   lt->SetSelected(true);

   int index = lt->AddLabel(region);

   mProject->PushState(_("Added label"), _("Label"));

   mProject->RedrawProject();
   mProject->GetTrackPanel()->EnsureVisible((Track *)lt);
   mProject->GetTrackPanel()->SetFocus();

   return index;
}

void TracksMenuCommands::OnEditLabels()
{
   wxString format = mProject->GetSelectionFormat();

   LabelDialog dlg(mProject, mProject->GetDirManager(), mProject->GetTracks(),
      mProject->GetViewInfo(), mProject->GetRate(), format);

   if (dlg.ShowModal() == wxID_OK) {
      mProject->PushState(_("Edited labels"), _("Label"));
      mProject->RedrawProject();
   }
}

void TracksMenuCommands::OnSortTime()
{
   SortTracks(kAudacitySortByTime);

   mProject->PushState(_("Tracks sorted by time"), _("Sort by Time"));

   mProject->GetTrackPanel()->Refresh(false);
}

void TracksMenuCommands::OnSortName()
{
   SortTracks(kAudacitySortByName);

   mProject->PushState(_("Tracks sorted by name"), _("Sort by Name"));

   mProject->GetTrackPanel()->Refresh(false);
}

double TracksMenuCommands::GetTime(Track *t)
{
   double stime = 0.0;

   if (t->GetKind() == Track::Wave) {
      WaveTrack *w = (WaveTrack *)t;
      stime = w->GetEndTime();

      WaveClip *c;
      int ndx;
      for (ndx = 0; ndx < w->GetNumClips(); ndx++) {
         c = w->GetClipByIndex(ndx);
         if (c->GetNumSamples() == 0)
            continue;
         if (c->GetStartTime() < stime) {
            stime = c->GetStartTime();
         }
      }
   }
   else if (t->GetKind() == Track::Label) {
      LabelTrack *l = (LabelTrack *)t;
      stime = l->GetStartTime();
   }

   return stime;
}

void TracksMenuCommands::SortTracks(int flags)
{
   int ndx = 0;
   int cmpValue;
   wxArrayPtrVoid arr;
   TrackListIterator iter(mProject->GetTracks());
   Track *track = iter.First();
   bool lastTrackLinked = false;
   //sort by linked tracks. Assumes linked track follows owner in list.
   while (track) {
      if(lastTrackLinked) {
         //insert after the last track since this track should be linked to it.
         ndx++;
      }
      else {
         for (ndx = 0; ndx < (int)arr.GetCount(); ndx++) {
            if(flags & kAudacitySortByName){
               //do case insensitive sort - cmpNoCase returns less than zero if the string is 'less than' its argument
               //also if we have case insensitive equality, then we need to sort by case as well
               //We sort 'b' before 'B' accordingly.  We uncharacteristically use greater than for the case sensitive
               //compare because 'b' is greater than 'B' in ascii.
               cmpValue = track->GetName().CmpNoCase(((Track *) arr[ndx])->GetName());
               if (cmpValue < 0 ||
                   (0 == cmpValue && track->GetName().CompareTo(((Track *) arr[ndx])->GetName()) > 0) )
                  break;
            }
            //sort by time otherwise
            else if(flags & kAudacitySortByTime){
               //we have to search each track and all its linked ones to fine the minimum start time.
               double time1,time2,tempTime;
               Track* tempTrack;
               int    candidatesLookedAt;

               candidatesLookedAt = 0;
               tempTrack = track;
               time1=time2=std::numeric_limits<double>::max(); //TODO: find max time value. (I don't think we have one yet)
               while(tempTrack){
                  tempTime = GetTime(tempTrack);
                  time1 = time1<tempTime? time1:tempTime;
                  if(tempTrack->GetLinked())
                     tempTrack = tempTrack->GetLink();
                  else
                     tempTrack = NULL;
               }

               //get candidate's (from sorted array) time
               tempTrack = (Track *) arr[ndx];
               while(tempTrack){
                  tempTime = GetTime(tempTrack);
                  time2 = time2<tempTime? time2:tempTime;
                  if(tempTrack->GetLinked() && (ndx+candidatesLookedAt <  (int)arr.GetCount()-1) ) {
                     candidatesLookedAt++;
                     tempTrack = (Track*) arr[ndx+candidatesLookedAt];
                  }
                  else
                     tempTrack = NULL;
               }

               if (time1 < time2)
                  break;

               ndx+=candidatesLookedAt;
            }
         }
      }
      arr.Insert(track, ndx);

      lastTrackLinked = track->GetLinked();
      track = iter.RemoveCurrent();
   }

   for (ndx = 0; ndx < (int)arr.GetCount(); ndx++) {
      mProject->GetTracks()->Add((Track *)arr[ndx]);
   }
}

//This will pop up the track panning dialog for specified track
void TracksMenuCommands::OnTrackPan()
{
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   Track *const track = trackPanel->GetFocusedTrack();
   if (!track || (track->GetKind() != Track::Wave)) {
      return;
   }

   LWSlider *slider = trackPanel->GetTrackInfo()->PanSlider
      (static_cast<WaveTrack*>(track));
   if (slider->ShowDialog()) {
      mProject->SetTrackPan(track, slider);
   }
}

void TracksMenuCommands::OnTrackPanLeft()
{
   TrackPanel *const trackPanel = mProject->GetTrackPanel();
   Track *const track = trackPanel->GetFocusedTrack();
   if (!track || (track->GetKind() != Track::Wave)) {
      return;
   }

   LWSlider *slider = trackPanel->GetTrackInfo()->PanSlider
      (static_cast<WaveTrack*>(track));
   slider->Decrease(1);
   mProject->SetTrackPan(track, slider);
}
