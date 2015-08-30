#include "TransportMenuCommands.h"

#include "../AudioIO.h"
#include "../Project.h"
#include "../commands/CommandManager.h"
#include "../toolbars/ControlToolBar.h"

#define FN(X) new ObjectCommandFunctor<TransportMenuCommands>(this, &TransportMenuCommands:: X )

TransportMenuCommands::TransportMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void TransportMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

   /* i18n-hint: (verb) Start or Stop audio playback*/
   c->AddItem(wxT("PlayStop"), _("Pl&ay/Stop"), FN(OnPlayStop), wxT("Space"),
      AlwaysEnabledFlag,
      AlwaysEnabledFlag);
}

void TransportMenuCommands::CreateNonMenuCommands(CommandManager *c)
{
   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

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
