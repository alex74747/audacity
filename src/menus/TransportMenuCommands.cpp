#include "../Experimental.h"
#include "TransportMenuCommands.h"

#include "../AudioIO.h"
#include "../DeviceManager.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../SoundActivatedRecord.h"
#include "../TimerRecordDialog.h"
#include "../TrackPanel.h"
#include "../commands/CommandManager.h"
#include "../toolbars/ControlToolBar.h"
#include "../toolbars/DeviceToolBar.h"
#include "../toolbars/MixerToolBar.h"

#define FN(X) new ObjectCommandFunctor<TransportMenuCommands>(this, &TransportMenuCommands:: X )

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
      c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

      /* i18n-hint: (verb) Start or Stop audio playback*/
      c->AddItem(wxT("PlayStop"), _("Pl&ay/Stop"), FN(OnPlayStop), wxT("Space"),
         AlwaysEnabledFlag,
         AlwaysEnabledFlag);
      c->AddItem(wxT("PlayStopSelect"), _("Play/Stop and &Set Cursor"), FN(OnPlayStopSelect), wxT("Shift+A"),
         AlwaysEnabledFlag,
         AlwaysEnabledFlag);
      c->AddItem(wxT("PlayLooped"), _("&Loop Play"), FN(OnPlayLooped), wxT("Shift+Space"),
         WaveTracksExistFlag | AudioIONotBusyFlag,
         WaveTracksExistFlag | AudioIONotBusyFlag);
      c->AddItem(wxT("Pause"), _("&Pause"), FN(OnPause), wxT("P"),
         AlwaysEnabledFlag,
         AlwaysEnabledFlag);
      c->AddItem(wxT("SkipStart"), _("S&kip to Start"), FN(OnSkipStart), wxT("Home"),
         AudioIONotBusyFlag,
         AudioIONotBusyFlag);
      c->AddItem(wxT("SkipEnd"), _("Skip to E&nd"), FN(OnSkipEnd), wxT("End"),
         WaveTracksExistFlag | AudioIONotBusyFlag,
         WaveTracksExistFlag | AudioIONotBusyFlag);

      c->AddSeparator();

      /* i18n-hint: (verb)*/
      c->AddItem(wxT("Record"), _("&Record"), FN(OnRecord), wxT("R"));
      c->AddItem(wxT("TimerRecord"), _("&Timer Record..."), FN(OnTimerRecord), wxT("Shift+T"));
      c->AddItem(wxT("RecordAppend"), _("Appen&d Record"), FN(OnRecordAppend), wxT("Shift+R"));

      c->AddSeparator();

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
   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

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
      //If this project isn't playing, but another one is, stop playing the old and start the new.

      //find out which project we need;
      AudacityProject* otherProject = NULL;
      for (unsigned i = 0; i<gAudacityProjects.GetCount(); i++) {
         if (gAudioIO->IsStreamActive(gAudacityProjects[i]->GetAudioIOToken())) {
            otherProject = gAudacityProjects[i];
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
   wxCommandEvent evt;
   ControlToolBar *toolbar = mProject->GetControlToolBar();

   //If busy, stop playing, make sure everything is unpaused.
   if (gAudioIO->IsStreamActive(mProject->GetAudioIOToken())) {
      toolbar->SetPlay(false);        //Pops
      toolbar->SetStop(true);         //Pushes stop down
      mProject->GetViewInfo().selectedRegion.setT0(gAudioIO->GetStreamTime(), false);
      mProject->ModifyState(false);           // without bWantsAutoSave
      toolbar->OnStop(evt);
   }
   else if (!gAudioIO->IsBusy()) {
      //Otherwise, start playing (assuming audio I/O isn't busy)
      //toolbar->SetPlay(true); // Not needed as set in PlayPlayRegion()
      toolbar->SetStop(false);

      // Will automatically set mLastPlayMode
      toolbar->PlayCurrentRegion(false);
   }
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

void TransportMenuCommands::OnTimerRecord()
{
   //we break the prompting and waiting dialogs into two sections
   //because they both give the user a chance to click cancel
   //and therefore remove the newly inserted track.

   TimerRecordDialog dialog(mProject /* parent */);
   int modalResult = dialog.ShowModal();
   if (modalResult == wxID_CANCEL)
   {
      // Cancelled before recording - don't need to do anyting.
   }
   else if (!dialog.RunWaitDialog())
   {
      //RunWaitDialog() shows the "wait for start" as well as "recording" dialog
      //if it returned false it means the user cancelled while the recording, so throw out the fresh track.
      //However, we can't undo it here because the PushState() is called in TrackPanel::OnTimer(),
      //which is blocked by this function.
      //so instead we mark a flag to undo it there.
      mProject->SetTimerRecordFlag();
   }
}

void TransportMenuCommands::OnRecordAppend()
{
   wxCommandEvent evt;
   evt.SetInt(1); // 0 is default, use 1 to set shift on, 2 to clear it

   mProject->GetControlToolBar()->OnRecord(evt);
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

   if (gAudioIO->IsStreamActive())
      mProject->GetControlToolBar()->OnStop(evt);
}

void TransportMenuCommands::OnPlayOneSecond()
{
   if (!MakeReadyToPlay())
      return;

   double pos = mProject->GetTrackPanel()->GetMostRecentXPos();
   mProject->mLastPlayMode = oneSecondPlay;
   mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(pos - 0.5, pos + 0.5), mProject->GetDefaultPlayOptions());
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
   mProject->mLastPlayMode = oneSecondPlay;

   // An alternative, commented out below, is to disable autoscroll
   // only when playing a short region, less than or equal to a second.
   //   mLastPlayMode = ((t1-t0) > 1.0) ? normalPlay : oneSecondPlay;

   mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t1), mProject->GetDefaultPlayOptions());
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

   mProject->mLastPlayMode = oneSecondPlay;      // this disables auto scrolling, as in OnPlayToSelection()

   mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0 - beforeLen, t0), mProject->GetDefaultPlayOptions());
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

   mProject->mLastPlayMode = oneSecondPlay;      // this disables auto scrolling, as in OnPlayToSelection()

   if (t1 - t0 > 0.0 && t1 - t0 < afterLen)
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t1), mProject->GetDefaultPlayOptions());
   else
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t0 + afterLen), mProject->GetDefaultPlayOptions());
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

   mProject->mLastPlayMode = oneSecondPlay;      // this disables auto scrolling, as in OnPlayToSelection()

   if (t1 - t0 > 0.0 && t1 - t0 < beforeLen)
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t1), mProject->GetDefaultPlayOptions());
   else
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t1 - beforeLen, t1), mProject->GetDefaultPlayOptions());
}

void TransportMenuCommands::OnPlayAfterSelectionEnd()
{
   if (!MakeReadyToPlay())
      return;

   ViewInfo &viewInfo = mProject->GetViewInfo();
   double t1 = viewInfo.selectedRegion.t1();
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   mProject->mLastPlayMode = oneSecondPlay;      // this disables auto scrolling, as in OnPlayToSelection()

   mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t1, t1 + afterLen), mProject->GetDefaultPlayOptions());
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

   mProject->mLastPlayMode = oneSecondPlay;      // this disables auto scrolling, as in OnPlayToSelection()

   if (t1 - t0 > 0.0 && t1 - t0 < afterLen)
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0 - beforeLen, t1), mProject->GetDefaultPlayOptions());
   else
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0 - beforeLen, t0 + afterLen), mProject->GetDefaultPlayOptions());
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

   mProject->mLastPlayMode = oneSecondPlay;      // this disables auto scrolling, as in OnPlayToSelection()

   if (t1 - t0 > 0.0 && t1 - t0 < beforeLen)
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t0, t1 + afterLen), mProject->GetDefaultPlayOptions());
   else
      mProject->GetControlToolBar()->PlayPlayRegion
      (SelectedRegion(t1 - beforeLen, t1 + afterLen), mProject->GetDefaultPlayOptions());
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

   toolbar->SetPlay(true, loop, cutpreview);
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
