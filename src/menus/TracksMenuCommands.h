#ifndef __AUDACITY_TRACKS_MENU_COMMANDS__
#define __AUDACITY_TRACKS_MENU_COMMANDS__

class AudacityProject;
class CommandManager;

class TracksMenuCommands
{
   TracksMenuCommands(const TracksMenuCommands&);
   TracksMenuCommands& operator= (const TracksMenuCommands&);
public:
   TracksMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);

private:
   void OnNewWaveTrack();
   void OnNewStereoTrack();
   void OnNewLabelTrack();
   void OnNewTimeTrack();
   void OnStereoToMono(int index);

   void OnMixAndRender();
   void OnMixAndRenderToNewTrack();
   void HandleMixAndRender(bool toNewTrack);

   void OnResample();
public:
   void OnRemoveTracks();
private:
   void OnMuteAllTracks();
   void OnUnMuteAllTracks();

   void OnAlignNoSync(int index);
   void OnAlign(int index);
   void OnAlignMoveSel(int index);
   void HandleAlign(int index, bool moveSel);
#ifdef EXPERIMENTAL_SCOREALIGN
   void OnScoreAlign();
#endif // EXPERIMENTAL_SCOREALIGN

   AudacityProject *const mProject;
   size_t mAlignLabelsCount;
};

#endif
