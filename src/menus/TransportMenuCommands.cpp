#include "../Audacity.h"
#include "../Experimental.h"
#include "TransportMenuCommands.h"

#include "../AudioIO.h"
#include "../DeviceManager.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../SoundActivatedRecord.h"
#include "../TimerRecordDialog.h"
#include "../TrackPanel.h"
#include "../UndoManager.h"
#include "../commands/CommandManager.h"
#include "../toolbars/ControlToolBar.h"
#include "../tracks/ui/Scrubbing.h"
#include "../toolbars/DeviceToolBar.h"
#include "../toolbars/MixerToolBar.h"
#include "../toolbars/TranscriptionToolBar.h"

#include "../TrackPanel.h"
#include "../tracks/ui/Scrubbing.h"
#include "../prefs/TracksPrefs.h"
#include "../widgets/Ruler.h"

#define FN(X) FNT(TransportMenuCommands, this, & TransportMenuCommands :: X)

TransportMenuCommands::TransportMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void TransportMenuCommands::Create(CommandManager *c)
{
   /*i18n-hint: 'Transport' is the name given to the set of controls that
   play, record, pause etc. */
   c->BeginMenu(_("T&ransport"));
   {
      c->SetDefaultFlags(CanStopAudioStreamFlag, CanStopAudioStreamFlag);

      /* i18n-hint: (verb) Start or Stop audio playback*/
      c->AddItem(wxT("PlayStop"), _("Pl&ay/Stop"), FN(OnPlayStop), wxT("Space"));
      c->AddItem(wxT("PlayStopSelect"), _("Play/Stop and &Set Cursor"), FN(OnPlayStopSelect), wxT("X"));
      c->AddItem(wxT("PlayLooped"), _("&Loop Play"), FN(OnPlayLooped), wxT("Shift+Space"),
         WaveTracksExistFlag | AudioIONotBusyFlag | CanStopAudioStreamFlag,
         WaveTracksExistFlag | AudioIONotBusyFlag | CanStopAudioStreamFlag);

      // Scrubbing sub-menu
      mProject->GetScrubber().AddMenuItems();

      c->AddItem(wxT("Pause"), _("&Pause"), FN(OnPause), wxT("P"));
      c->AddItem(wxT("SkipStart"), _("S&kip to Start"), FN(OnSkipStart), wxT("Home"),
         AudioIONotBusyFlag,
         AudioIONotBusyFlag);
      c->AddItem(wxT("SkipEnd"), _("Skip to E&nd"), FN(OnSkipEnd), wxT("End"),
         WaveTracksExistFlag | AudioIONotBusyFlag,
         WaveTracksExistFlag | AudioIONotBusyFlag);

      c->AddSeparator();

      c->SetDefaultFlags(AudioIONotBusyFlag | CanStopAudioStreamFlag,
                         AudioIONotBusyFlag | CanStopAudioStreamFlag);


      c->AddSeparator();

      c->AddCheck(wxT("PinnedHead"), _("Pinned Recording/Playback &Head"),
                  FN(OnTogglePinnedHead), 0,
                  // Switching of scrolling on and off is permitted even during transport
                  AlwaysEnabledFlag, AlwaysEnabledFlag);

      c->AddCheck(wxT("Duplex"), _("&Overdub (on/off)"), FN(OnTogglePlayRecording), 0);
      c->AddCheck(wxT("SWPlaythrough"), _("So&ftware Playthrough (on/off)"), FN(OnToggleSWPlaythrough), 0);

      // Sound Activated recording options
      c->AddCheck(wxT("SoundActivation"), _("Sound A&ctivated Recording (on/off)"), FN(OnToggleSoundActivated), 0);
      c->AddItem(wxT("SoundActivationLevel"), _("Sound Activation Le&vel..."), FN(OnSoundActivated));

#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
      c->AddCheck(wxT("AutomatedInputLevelAdjustmentOnOff"), _("A&utomated Recording Level Adjustment (on/off)"), FN(OnToggleAutomatedInputLevelAdjustment), 0);
#endif

      c->AddItem(wxT("RescanDevices"), _("R&escan Audio Devices"), FN(OnRescanDevices));
   }
   c->EndMenu();
}

void TransportMenuCommands::CreateNonMenuCommands(CommandManager *c)
{
   c->SetDefaultFlags(CanStopAudioStreamFlag, CanStopAudioStreamFlag);

   /* i18n-hint: (verb) Start playing audio*/
   c->AddCommand(wxT("Play"), _("Play"), FN(OnPlayStop),
      WaveTracksExistFlag | AudioIONotBusyFlag,
      WaveTracksExistFlag | AudioIONotBusyFlag);
   /* i18n-hint: (verb) Stop playing audio*/
   c->AddCommand(wxT("Stop"), _("Stop"), FN(OnStop),
      AudioIOBusyFlag,
      AudioIOBusyFlag);

   c->SetDefaultFlags(CaptureNotBusyFlag, CaptureNotBusyFlag);

   c->AddCommand(wxT("PlayOneSec"), _("Play One Second"), FN(OnPlayOneSecond), wxT("1"),
      CaptureNotBusyFlag,
      CaptureNotBusyFlag);
   c->AddCommand(wxT("PlayToSelection"), _("Play To Selection"), FN(OnPlayToSelection), wxT("B"),
      CaptureNotBusyFlag,
      CaptureNotBusyFlag);
   c->AddCommand(wxT("PlayBeforeSelectionStart"), _("Play Before Selection Start"), FN(OnPlayBeforeSelectionStart), wxT("Shift+F5"));
   c->AddCommand(wxT("PlayAfterSelectionStart"), _("Play After Selection Start"), FN(OnPlayAfterSelectionStart), wxT("Shift+F6"));
   c->AddCommand(wxT("PlayBeforeSelectionEnd"), _("Play Before Selection End"), FN(OnPlayBeforeSelectionEnd), wxT("Shift+F7"));
   c->AddCommand(wxT("PlayAfterSelectionEnd"), _("Play After Selection End"), FN(OnPlayAfterSelectionEnd), wxT("Shift+F8"));
   c->AddCommand(wxT("PlayBeforeAndAfterSelectionStart"), _("Play Before and After Selection Start"), FN(OnPlayBeforeAndAfterSelectionStart), wxT("Ctrl+Shift+F5"));
   c->AddCommand(wxT("PlayBeforeAndAfterSelectionEnd"), _("Play Before and After Selection End"), FN(OnPlayBeforeAndAfterSelectionEnd), wxT("Ctrl+Shift+F7"));
   c->AddCommand(wxT("PlayCutPreview"), _("Play Cut Preview"), FN(OnPlayCutPreview), wxT("C"),
      CaptureNotBusyFlag,
      CaptureNotBusyFlag);

   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddCommand(wxT("InputDevice"), _("Change recording device"), FN(OnInputDevice), wxT("Shift+I"),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);
   c->AddCommand(wxT("OutputDevice"), _("Change playback device"), FN(OnOutputDevice), wxT("Shift+O"),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);
   c->AddCommand(wxT("AudioHost"), _("Change audio host"), FN(OnAudioHost), wxT("Shift+H"),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);
   c->AddCommand(wxT("InputChannels"), _("Change recording channels"), FN(OnInputChannels), wxT("Shift+N"),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);

   c->AddCommand(wxT("OutputGain"), _("Adjust playback volume"), FN(OnOutputGain));
   c->AddCommand(wxT("OutputGainInc"), _("Increase playback volume"), FN(OnOutputGainInc));
   c->AddCommand(wxT("OutputGainDec"), _("Decrease playback volume"), FN(OnOutputGainDec));
   c->AddCommand(wxT("InputGain"), _("Adjust recording volume"), FN(OnInputGain));
   c->AddCommand(wxT("InputGainInc"), _("Increase recording volume"), FN(OnInputGainInc));
   c->AddCommand(wxT("InputGainDec"), _("Decrease recording volume"), FN(OnInputGainDec));

   c->SetDefaultFlags(CaptureNotBusyFlag, CaptureNotBusyFlag);

   c->AddCommand(wxT("PlayAtSpeed"), _("Play at speed"), FN(OnPlayAtSpeed));
   c->AddCommand(wxT("PlayAtSpeedLooped"), _("Loop Play at speed"), FN(OnPlayAtSpeedLooped));
   c->AddCommand(wxT("PlayAtSpeedCutPreview"), _("Play Cut Preview at speed"), FN(OnPlayAtSpeedCutPreview));
   c->AddCommand(wxT("SetPlaySpeed"), _("Adjust playback speed"), FN(OnSetPlaySpeed));
   c->AddCommand(wxT("PlaySpeedInc"), _("Increase playback speed"), FN(OnPlaySpeedInc));
}

void TransportMenuCommands::OnPlayStop()
{
   ControlToolBar *toolbar = mProject->GetControlToolBar();

   //If this project is playing, stop playing, make sure everything is unpaused.
   if (gAudioIO->IsStreamActive(mProject->GetAudioIOToken())) {
      toolbar->SetPlay(false);        //Pops
      toolbar->SetStop(true);         //Pushes stop down
      toolbar->StopPlaying();
   }
   else if (gAudioIO->IsStreamActive()) {
      //If this project isn't playing, but another one is, stop playing the old and start the NEW.

      //find out which project we need;
      AudacityProject* otherProject = NULL;
      for (unsigned i = 0; i<gAudacityProjects.size(); i++) {
         if (gAudioIO->IsStreamActive(gAudacityProjects[i]->GetAudioIOToken())) {
            otherProject = gAudacityProjects[i].get();
            break;
         }
      }

      //stop playing the other project
      if (otherProject) {
         ControlToolBar *otherToolbar = otherProject->GetControlToolBar();
         otherToolbar->SetPlay(false);        //Pops
         otherToolbar->SetStop(true);         //Pushes stop down
         otherToolbar->StopPlaying();
      }

      //play the front project
      if (!gAudioIO->IsBusy()) {
         //update the playing area
         mProject->TP_DisplaySelection();
         //Otherwise, start playing (assuming audio I/O isn't busy)
         //toolbar->SetPlay(true); // Not needed as done in PlayPlayRegion.
         toolbar->SetStop(false);

         // Will automatically set mLastPlayMode
         toolbar->PlayCurrentRegion(false);
      }
   }
   else if (!gAudioIO->IsBusy()) {
      //Otherwise, start playing (assuming audio I/O isn't busy)
      //toolbar->SetPlay(true); // Not needed as done in PlayPlayRegion.
      toolbar->SetStop(false);

      // Will automatically set mLastPlayMode
      toolbar->PlayCurrentRegion(false);
   }
}

// The code for "OnPlayStopSelect" is simply the code of "OnPlayStop" and "OnStopSelect" merged.
void TransportMenuCommands::OnPlayStopSelect()
{
   ControlToolBar *toolbar = mProject->GetControlToolBar();
   wxCommandEvent evt;
   if (DoPlayStopSelect(false, false))
      toolbar->OnStop(evt);
   else if (!gAudioIO->IsBusy()) {
      //Otherwise, start playing (assuming audio I/O isn't busy)
      //toolbar->SetPlay(true); // Not needed as set in PlayPlayRegion()
      toolbar->SetStop(false);

      // Will automatically set mLastPlayMode
      toolbar->PlayCurrentRegion(false);
   }
}

bool TransportMenuCommands::DoPlayStopSelect(bool click, bool shift)
{
   ControlToolBar *toolbar = mProject->GetControlToolBar();

   //If busy, stop playing, make sure everything is unpaused.
   if (mProject->GetScrubber().HasStartedScrubbing() ||
       gAudioIO->IsStreamActive(mProject->GetAudioIOToken())) {
      toolbar->SetPlay(false);        //Pops
      toolbar->SetStop(true);         //Pushes stop down

      // change the selection
      auto time = gAudioIO->GetStreamTime();
      auto &selection = mProject->GetViewInfo().selectedRegion;
      if (shift && click) {
         // Change the region selection, as if by shift-click at the play head
         auto t0 = selection.t0(), t1 = selection.t1();
         if (time < t0)
            // Grow selection
            t0 = time;
         else if (time > t1)
            // Grow selection
            t1 = time;
         else {
            // Shrink selection, changing the nearer boundary
            if (fabs(t0 - time) < fabs(t1 - time))
               t0 = time;
            else
               t1 = time;
         }
         selection.setTimes(t0, t1);
      }
      else if (click) {
         // avoid a point at negative time.
         time = wxMax( time, 0 );
         // Set a point selection, as if by a click at the play head
         selection.setTimes(time, time);
      } else
         // How stop and set cursor always worked
         // -- change t0, collapsing to point only if t1 was greater
         selection.setT0(time, false);

      mProject->ModifyState(false);           // without bWantsAutoSave
      return true;
   }
   return false;
}

void TransportMenuCommands::OnPlayLooped()
{
   if (!MakeReadyToPlay(true))
      return;

   // Now play in a loop
   // Will automatically set mLastPlayMode
   mProject->GetControlToolBar()->PlayCurrentRegion(true);
}

void TransportMenuCommands::OnPause()
{
   wxCommandEvent evt;

   mProject->GetControlToolBar()->OnPause(evt);
}

void TransportMenuCommands::OnSkipStart()
{
   wxCommandEvent evt;

   mProject->GetControlToolBar()->OnRewind(evt);
   mProject->ModifyState(false);
}

void TransportMenuCommands::OnSkipEnd()
{
   wxCommandEvent evt;

   mProject->GetControlToolBar()->OnFF(evt);
   mProject->ModifyState(false);
}

void TransportMenuCommands::OnRecord()
{
   wxCommandEvent evt;
   evt.SetInt(2); // 0 is default, use 1 to set shift on, 2 to clear it

   mProject->GetControlToolBar()->OnRecord(evt);
}

// Post Timer Recording Actions
// Ensure this matches the enum in TimerRecordDialog.cpp
enum {
   POST_TIMER_RECORD_STOPPED = -3,
   POST_TIMER_RECORD_CANCEL_WAIT,
   POST_TIMER_RECORD_CANCEL,
   POST_TIMER_RECORD_NOTHING,
   POST_TIMER_RECORD_CLOSE,
   POST_TIMER_RECORD_RESTART,
   POST_TIMER_RECORD_SHUTDOWN
};

void TransportMenuCommands::OnTimerRecord()
{
   auto undoManger = mProject->GetUndoManager();

   // MY: Due to improvements in how Timer Recording saves and/or exports
   // it is now safer to disable Timer Recording when there is more than
   // one open project.
   if (mProject->GetOpenProjectCount() > 1) {
      ::wxMessageBox(_("Timer Recording cannot be used with more than one open project.\n\nPlease close any additional projects and try again."),
         _("Timer Recording"),
         wxICON_INFORMATION | wxOK);
      return;
   }

   // MY: If the project has unsaved changes then we no longer allow access
   // to Timer Recording.  This decision has been taken as the safest approach
   // preventing issues surrounding "dirty" projects when Automatic Save/Export
   // is used in Timer Recording.
  if ((undoManger->UnsavedChanges()) && (mProject->ProjectHasTracks() || mProject->EmptyCanBeDirty())) {
     wxMessageBox(_("Timer Recording cannot be used while you have unsaved changes.\n\nPlease save or close this project and try again."),
                         _("Timer Recording"),
                   wxICON_INFORMATION | wxOK);
      return;
   }
   // We use this variable to display "Current Project" in the Timer Recording save project field
   bool bProjectSaved = mProject->IsProjectSaved();

   //we break the prompting and waiting dialogs into two sections
   //because they both give the user a chance to click cancel
   //and therefore remove the newly inserted track.

   TimerRecordDialog dialog(mProject, bProjectSaved); /* parent, project saved? */
   int modalResult = dialog.ShowModal();
   if (modalResult == wxID_CANCEL)
   {
      // Cancelled before recording - don't need to do anyting.
   }
   else
   {
      int iTimerRecordingOutcome = dialog.RunWaitDialog();
      switch (iTimerRecordingOutcome) {
      case POST_TIMER_RECORD_CANCEL_WAIT:
         // Canceled on the wait dialog
         mProject->RollbackState();
         break;
      case POST_TIMER_RECORD_CANCEL:
         // RunWaitDialog() shows the "wait for start" as well as "recording" dialog
         // if it returned POST_TIMER_RECORD_CANCEL it means the user cancelled while the recording, so throw out the fresh track.
         // However, we can't undo it here because the PushState() is called in TrackPanel::OnTimer(),
         // which is blocked by this function.
         // so instead we mark a flag to undo it there.
         mProject->SetTimerRecordFlag();
      break;
      case POST_TIMER_RECORD_NOTHING:
         // No action required
      break;
      case POST_TIMER_RECORD_CLOSE:
         // Quit Audacity
         exit(0);
      break;
      case POST_TIMER_RECORD_RESTART:
         // Restart System
#ifdef __WINDOWS__
         system("shutdown /r /f /t 30");
#endif
      break;
      case POST_TIMER_RECORD_SHUTDOWN:
         // Shutdown System
#ifdef __WINDOWS__
         system("shutdown /s /f /t 30");
#endif
      break;
      }
   }
}

void TransportMenuCommands::OnRecordAppend()
{
   wxCommandEvent evt;
   evt.SetInt(1); // 0 is default, use 1 to set shift on, 2 to clear it

   mProject->GetControlToolBar()->OnRecord(evt);
}

void TransportMenuCommands::OnTogglePinnedHead()
{
   bool value = !TracksPrefs::GetPinnedHeadPreference();
   TracksPrefs::SetPinnedHeadPreference(value, true);
   mProject->ModifyAllProjectToolbarMenus();

   // Change what happens in case transport is in progress right now
   auto ctb = GetActiveProject()->GetControlToolBar();
   if (ctb)
      ctb->StartScrollingIfPreferred();

   auto ruler = mProject->GetRulerPanel();
   if (ruler)
      // Update button image
      ruler->UpdateButtonStates();

   auto &scrubber = mProject->GetScrubber();
   if (scrubber.HasStartedScrubbing())
      scrubber.SetScrollScrubbing(value);
}

void TransportMenuCommands::OnTogglePlayRecording()
{
   bool Duplex;
   gPrefs->Read(wxT("/AudioIO/Duplex"), &Duplex, true);
   gPrefs->Write(wxT("/AudioIO/Duplex"), !Duplex);
   gPrefs->Flush();
   mProject->ModifyAllProjectToolbarMenus();
}

void TransportMenuCommands::OnToggleSWPlaythrough()
{
   bool SWPlaythrough;
   gPrefs->Read(wxT("/AudioIO/SWPlaythrough"), &SWPlaythrough, false);
   gPrefs->Write(wxT("/AudioIO/SWPlaythrough"), !SWPlaythrough);
   gPrefs->Flush();
   mProject->ModifyAllProjectToolbarMenus();
}

void TransportMenuCommands::OnToggleSoundActivated()
{
   bool pause;
   gPrefs->Read(wxT("/AudioIO/SoundActivatedRecord"), &pause, false);
   gPrefs->Write(wxT("/AudioIO/SoundActivatedRecord"), !pause);
   gPrefs->Flush();
   mProject->ModifyAllProjectToolbarMenus();
}

void TransportMenuCommands::OnSoundActivated()
{
   SoundActivatedRecord dialog(mProject /* parent */);
   dialog.ShowModal();
}

#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
void TransportMenuCommands::OnToggleAutomatedInputLevelAdjustment()
{
   bool AVEnabled;
   gPrefs->Read(wxT("/AudioIO/AutomatedInputLevelAdjustment"), &AVEnabled, false);
   gPrefs->Write(wxT("/AudioIO/AutomatedInputLevelAdjustment"), !AVEnabled);
   gPrefs->Flush();
   mProject->ModifyAllProjectToolbarMenus();
}
#endif

void TransportMenuCommands::OnRescanDevices()
{
   DeviceManager::Instance()->Rescan();
}

void TransportMenuCommands::OnStop()
{
   wxCommandEvent evt;

   mProject->GetControlToolBar()->OnStop(evt);
}

void TransportMenuCommands::OnPlayOneSecond()
{
   if (!MakeReadyToPlay())
      return;

   double pos = mProject->GetTrackPanel()->GetMostRecentXPos();
   mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(pos - 0.5, pos + 0.5), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
}

/// The idea for this function (and first implementation)
/// was from Juhana Sadeharju.  The function plays the
/// sound between the current mouse position and the
/// nearest selection boundary.  This gives four possible
/// play regions depending on where the current mouse
/// position is relative to the left and right boundaries
/// of the selection region.
void TransportMenuCommands::OnPlayToSelection()
{
   if (!MakeReadyToPlay())
      return;

   double pos = mProject->GetTrackPanel()->GetMostRecentXPos();

   ViewInfo &viewInfo = mProject->GetViewInfo();
   double t0, t1;
   // check region between pointer and the nearest selection edge
   if (fabs(pos - viewInfo.selectedRegion.t0()) <
      fabs(pos - viewInfo.selectedRegion.t1())) {
      t0 = t1 = viewInfo.selectedRegion.t0();
   }
   else {
      t0 = t1 = viewInfo.selectedRegion.t1();
   }
   if (pos < t1)
      t0 = pos;
   else
      t1 = pos;

   // JKC: oneSecondPlay mode disables auto scrolling
   // On balance I think we should always do this in this function
   // since you are typically interested in the sound EXACTLY
   // where the cursor is.
   // TODO: have 'playing attributes' such as 'with_autoscroll'
   // rather than modes, since that's how we're now using the modes.

   // An alternative, commented out below, is to disable autoscroll
   // only when playing a short region, less than or equal to a second.
   //   mLastPlayMode = ((t1-t0) > 1.0) ? normalPlay : oneSecondPlay;

   mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t1), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
}

// The next functions provide a limited version of the
// functionality of OnPlayToSelection() for keyboard users

void TransportMenuCommands::OnPlayBeforeSelectionStart()
{
   if (!MakeReadyToPlay())
      return;

   double t0 = mProject->GetViewInfo().selectedRegion.t0();
   double beforeLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);

   mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0 - beforeLen, t0), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
}

void TransportMenuCommands::OnPlayAfterSelectionStart()
{
   if (!MakeReadyToPlay())
      return;

   ViewInfo &viewInfo = mProject->GetViewInfo();
   double t0 = viewInfo.selectedRegion.t0();
   double t1 = viewInfo.selectedRegion.t1();
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   if (t1 - t0 > 0.0 && t1 - t0 < afterLen)
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t1), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
   else
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t0 + afterLen), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
}

void TransportMenuCommands::OnPlayBeforeSelectionEnd()
{
   if (!MakeReadyToPlay())
      return;

   ViewInfo &viewInfo = mProject->GetViewInfo();
   double t0 = viewInfo.selectedRegion.t0();
   double t1 = viewInfo.selectedRegion.t1();
   double beforeLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);

   if (t1 - t0 > 0.0 && t1 - t0 < beforeLen)
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t1), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
   else
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t1 - beforeLen, t1), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
}

void TransportMenuCommands::OnPlayAfterSelectionEnd()
{
   if (!MakeReadyToPlay())
      return;

   ViewInfo &viewInfo = mProject->GetViewInfo();
   double t1 = viewInfo.selectedRegion.t1();
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t1, t1 + afterLen), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
}

void TransportMenuCommands::OnPlayBeforeAndAfterSelectionStart()
{
   if (!MakeReadyToPlay())
      return;

   ViewInfo &viewInfo = mProject->GetViewInfo();
   double t0 = viewInfo.selectedRegion.t0();
   double t1 = viewInfo.selectedRegion.t1();
   double beforeLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   if (t1 - t0 > 0.0 && t1 - t0 < afterLen)
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0 - beforeLen, t1), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
   else
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0 - beforeLen, t0 + afterLen), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
}

void TransportMenuCommands::OnPlayBeforeAndAfterSelectionEnd()
{
   if (!MakeReadyToPlay())
      return;

   ViewInfo &viewInfo = mProject->GetViewInfo();
   double t0 = viewInfo.selectedRegion.t0();
   double t1 = viewInfo.selectedRegion.t1();
   double beforeLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   if (t1 - t0 > 0.0 && t1 - t0 < beforeLen)
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t1 + afterLen), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
   else
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t1 - beforeLen, t1 + afterLen), mProject->GetDefaultPlayOptions(),
       PlayMode::oneSecondPlay);
}

void TransportMenuCommands::OnPlayCutPreview()
{
   if (!MakeReadyToPlay(false, true))
      return;

   // Play with cut preview
   mProject->GetControlToolBar()->PlayCurrentRegion(false, true);
}

/// MakeReadyToPlay stops whatever is currently playing
/// and pops the play button up.  Then, if nothing is now
/// playing, it pushes the play button down and enables
/// the stop button.
bool TransportMenuCommands::MakeReadyToPlay(bool loop, bool cutpreview)
{
   ControlToolBar *toolbar = mProject->GetControlToolBar();
   wxCommandEvent evt;

   // If this project is playing, stop playing
   if (gAudioIO->IsStreamActive(mProject->GetAudioIOToken())) {
      toolbar->SetPlay(false);        //Pops
      toolbar->SetStop(true);         //Pushes stop down
      toolbar->OnStop(evt);

      ::wxMilliSleep(100);
   }

   // If it didn't stop playing quickly, or if some other
   // project is playing, return
   if (gAudioIO->IsBusy())
      return false;

   ControlToolBar::PlayAppearance appearance =
      cutpreview ? ControlToolBar::PlayAppearance::CutPreview
      : loop ? ControlToolBar::PlayAppearance::Looped
      : ControlToolBar::PlayAppearance::Straight;
   toolbar->SetPlay(true, appearance);
   toolbar->SetStop(false);

   return true;
}

void TransportMenuCommands::OnInputDevice()
{
   DeviceToolBar *tb = mProject->GetDeviceToolBar();
   if (tb) {
      tb->ShowInputDialog();
   }
}

void TransportMenuCommands::OnOutputDevice()
{
   DeviceToolBar *tb = mProject->GetDeviceToolBar();
   if (tb) {
      tb->ShowOutputDialog();
   }
}

void TransportMenuCommands::OnAudioHost()
{
   DeviceToolBar *tb = mProject->GetDeviceToolBar();
   if (tb) {
      tb->ShowHostDialog();
   }
}

void TransportMenuCommands::OnInputChannels()
{
   DeviceToolBar *tb = mProject->GetDeviceToolBar();
   if (tb) {
      tb->ShowChannelsDialog();
   }
}

void TransportMenuCommands::OnOutputGain()
{
   MixerToolBar *tb = mProject->GetMixerToolBar();
   if (tb) {
      tb->ShowOutputGainDialog();
   }
}

void TransportMenuCommands::OnOutputGainInc()
{
   MixerToolBar *tb = mProject->GetMixerToolBar();
   if (tb) {
      tb->AdjustOutputGain(1);
   }
}

void TransportMenuCommands::OnOutputGainDec()
{
   MixerToolBar *tb = mProject->GetMixerToolBar();
   if (tb) {
      tb->AdjustOutputGain(-1);
   }
}

void TransportMenuCommands::OnInputGain()
{
   MixerToolBar *tb = mProject->GetMixerToolBar();
   if (tb) {
      tb->ShowInputGainDialog();
   }
}

void TransportMenuCommands::OnInputGainInc()
{
   MixerToolBar *tb = mProject->GetMixerToolBar();
   if (tb) {
      tb->AdjustInputGain(1);
   }
}

void TransportMenuCommands::OnInputGainDec()
{
   MixerToolBar *tb = mProject->GetMixerToolBar();
   if (tb) {
      tb->AdjustInputGain(-1);
   }
}

void TransportMenuCommands::OnPlayAtSpeed()
{
   TranscriptionToolBar *tb = mProject->GetTranscriptionToolBar();
   if (tb) {
      tb->PlayAtSpeed(false, false);
   }
}

void TransportMenuCommands::OnPlayAtSpeedLooped()
{
   TranscriptionToolBar *tb = mProject->GetTranscriptionToolBar();
   if (tb) {
      tb->PlayAtSpeed(true, false);
   }
}

void TransportMenuCommands::OnPlayAtSpeedCutPreview()
{
   TranscriptionToolBar *tb = mProject->GetTranscriptionToolBar();
   if (tb) {
      tb->PlayAtSpeed(false, true);
   }
}

void TransportMenuCommands::OnSetPlaySpeed()
{
   TranscriptionToolBar *tb = mProject->GetTranscriptionToolBar();
   if (tb) {
      tb->ShowPlaySpeedDialog();
   }
}

void TransportMenuCommands::OnPlaySpeedInc()
{
   TranscriptionToolBar *tb = mProject->GetTranscriptionToolBar();
   if (tb) {
      tb->AdjustPlaySpeed(0.1f);
   }
}
