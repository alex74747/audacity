#include "TracksMenuCommands.h"

#include "../LabelTrack.h"
#include "../Project.h"
#include "../TimeTrack.h"
#include "../TrackPanel.h"
#include "../WaveTrack.h"
#include "../commands/CommandManager.h"

#define FN(X) new ObjectCommandFunctor<TracksMenuCommands>(this, &TracksMenuCommands:: X )

TracksMenuCommands::TracksMenuCommands(AudacityProject *project)
   : mProject(project)
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
