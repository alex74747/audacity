#include "../Audacity.h"
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

#define FN(X) FNT(TracksMenuCommands, this, & TracksMenuCommands :: X)

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

   {
      // Stereo to Mono is an oddball command that is also subject to control by the
      // plug-in manager, as if an effect.  Decide whether to show or hide it.
      const PluginID ID = EffectManager::Get().GetEffectByIdentifier(wxT("StereoToMono"));
      const PluginDescriptor *plug = PluginManager::Get().GetPlugin(ID);
      if (plug && plug->IsEnabled())
         c->AddItem(wxT("Stereo to Mono"), _("Stereo Trac&k to Mono"), FN(OnStereoToMono),
            AudioIONotBusyFlag | StereoRequiredFlag | WaveTracksSelectedFlag,
            AudioIONotBusyFlag | StereoRequiredFlag | WaveTracksSelectedFlag);
   }

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
}

void TracksMenuCommands::OnNewWaveTrack()
{
   auto t = mProject->GetTracks()->Add(
      mProject->GetTrackFactory()->NewWaveTrack(
         mProject->GetDefaultFormat(), mProject->GetRate()));
   mProject->SelectNone();

   t->SetSelected(true);

   mProject->PushState(_("Created new audio track"), _("New Track"));

   mProject->RedrawProject();
   mProject->GetTrackPanel()->EnsureVisible(t);
}

void TracksMenuCommands::OnNewStereoTrack()
{
   auto t = mProject->GetTracks()->Add(
      mProject->GetTrackFactory()->NewWaveTrack(
         mProject->GetDefaultFormat(), mProject->GetRate()));
   t->SetChannel(Track::LeftChannel);
   mProject->SelectNone();

   t->SetSelected(true);
   t->SetLinked(true);

   t = mProject->GetTracks()->Add(
      mProject->GetTrackFactory()->NewWaveTrack(
         mProject->GetDefaultFormat(), mProject->GetRate()));
   t->SetChannel(Track::RightChannel);

   t->SetSelected(true);

   mProject->PushState(_("Created new stereo audio track"), _("New Track"));

   mProject->RedrawProject();
   mProject->GetTrackPanel()->EnsureVisible(t);
}

void TracksMenuCommands::OnNewLabelTrack()
{
   auto t = mProject->GetTracks()->Add(
      mProject->GetTrackFactory()->NewLabelTrack());

   mProject->SelectNone();

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

   auto t = mProject->GetTracks()->AddToHead(
      mProject->GetTrackFactory()->NewTimeTrack());

   mProject->SelectNone();

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
   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);

   WaveTrack::Holder uNewLeft, uNewRight;
   MixAndRender(mProject->GetTracks(), mProject->GetTrackFactory(), mProject->GetRate(), mProject->GetDefaultFormat(), 0.0, 0.0, uNewLeft, uNewRight);

   if (uNewLeft) {

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
               t = iter.RemoveCurrent();
            }
            else {
               t = iter.Next();
            };
         }
         else
            t = iter.Next();
      }

      // Add NEW tracks

      auto pNewLeft = mProject->GetTracks()->Add(std::move(uNewLeft));
      decltype(pNewLeft) pNewRight{};
      if (uNewRight)
         pNewRight = mProject->GetTracks()->Add(std::move(uNewRight));

      // If we're just rendering (not mixing), keep the track name the same
      if (selectedCount == 1) {
         pNewLeft->SetName(firstName);
         if (pNewRight)
            pNewRight->SetName(firstName);
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
         if (pNewRight)
            msg.Printf(_("Mixed and rendered %d tracks into one new stereo track"),
            selectedCount);
         else
            msg.Printf(_("Mixed and rendered %d tracks into one new mono track"),
            selectedCount);
         mProject->PushState(msg, _("Mix and Render"));
      }

      mProject->GetTrackPanel()->SetFocus();
      mProject->GetTrackPanel()->SetFocusedTrack(pNewLeft);
      mProject->GetTrackPanel()->EnsureVisible(pNewLeft);
      mProject->RedrawProject();
   }
}

void TracksMenuCommands::OnResample()
{
   TrackListIterator iter(mProject->GetTracks());

   int newRate;

   while (true)
   {
      wxDialogWrapper dlg(mProject, wxID_ANY, wxString(_("Resample")));
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
         t = iter.RemoveCurrent();
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
