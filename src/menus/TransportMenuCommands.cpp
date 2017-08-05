#include "../Audacity.h"
#include "TransportMenuCommands.h"

#include "../AudioIO.h"
#include "../Project.h"
#include "../TimerRecordDialog.h"
#include "../UndoManager.h"
#include "../commands/CommandManager.h"
#include "../toolbars/ControlToolBar.h"
#include "../tracks/ui/Scrubbing.h"

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

   /* i18n-hint: (verb)*/
   c->AddItem(wxT("Record"), _("&Record"), FN(OnRecord), wxT("R"));
   c->AddItem(wxT("TimerRecord"), _("&Timer Record..."), FN(OnTimerRecord), wxT("Shift+T"));
   c->AddItem(wxT("RecordAppend"), _("Appen&d Record"), FN(OnRecordAppend), wxT("Shift+R"));

   c->AddSeparator();

   c->AddCheck(wxT("PinnedHead"), _("Pinned Recording/Playback &Head"),
               FN(OnTogglePinnedHead), 0,
               // Switching of scrolling on and off is permitted even during transport
               AlwaysEnabledFlag, AlwaysEnabledFlag);
}

void TransportMenuCommands::CreateNonMenuCommands(CommandManager *c)
{
   c->SetDefaultFlags(CanStopAudioStreamFlag, CanStopAudioStreamFlag);

   /* i18n-hint: (verb) Start playing audio*/
   c->AddCommand(wxT("Play"), _("Play"), FN(OnPlayStop),
      WaveTracksExistFlag | AudioIONotBusyFlag,
      WaveTracksExistFlag | AudioIONotBusyFlag);
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
   if (!mProject->MakeReadyToPlay(true))
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
