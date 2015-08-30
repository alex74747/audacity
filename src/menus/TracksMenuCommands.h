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

   AudacityProject *const mProject;
};

#endif
