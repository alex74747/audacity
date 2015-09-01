#ifndef __AUDACITY_TRACKS_MENU_COMMANDS__
#define __AUDACITY_TRACKS_MENU_COMMANDS__

class AudacityProject;
class CommandManager;
class LabelTrack;
class LWSlider;
class SelectedRegion;
class Track;
class WaveTrack;

#include <stddef.h>

class TracksMenuCommands
{
   TracksMenuCommands(const TracksMenuCommands&);
   TracksMenuCommands& operator= (const TracksMenuCommands&);
public:
   TracksMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);
   void CreateNonMenuCommands(CommandManager *c);

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

public:
   void OnSyncLock();

private:
   void OnAddLabel();
   void OnAddLabelPlaying();
public:
   //Adds label and returns index of label in labeltrack.
   int DoAddLabel(const SelectedRegion& region, bool preserveFocus = false);
   void DoEditLabels(LabelTrack *lt = nullptr, int index = -1);
private:
   void OnEditLabels();
   void OnToggleTypeToCreateLabel();

   void OnSortTime();
   void OnSortName();
   double GetTime(const Track *t);
   enum {
      kAudacitySortByTime = (1 << 1),
      kAudacitySortByName = (1 << 2)
   };
   void SortTracks(int flags);

   // non-menu commands
   void OnTrackPan();
   void OnTrackPanLeft();
   void OnTrackPanRight();
   void SetTrackPan(WaveTrack *wt, LWSlider * slider);

   void OnTrackGain();
   void OnTrackGainInc();
   void OnTrackGainDec();
   void SetTrackGain(WaveTrack * wt, LWSlider * slider);

   void OnTrackMenu();
   void OnTrackMute();
   void OnTrackSolo();
   void OnTrackClose();
   void OnTrackMoveUp();

   AudacityProject *const mProject;
   size_t mAlignLabelsCount;
};

#endif
