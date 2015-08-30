#include "TracksMenuCommands.h"

#include <wx/combobox.h>

#include "../LabelTrack.h"
#include "../Mix.h"
#include "../MixerBoard.h"
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
