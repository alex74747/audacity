#ifndef __AUDACITY_CURSOR_AND_FOCUS_COMMANDS__
#define __AUDACITY_CURSOR_AND_FOCUS_COMMANDS__

#include "../SelectedRegion.h"

class AudacityProject;
class CommandManager;

// There is no menu just for these commands.
// This class splits up the many edit commmands.
class CursorAndFocusCommands
{
   CursorAndFocusCommands(const CursorAndFocusCommands&);
   CursorAndFocusCommands& operator= (const CursorAndFocusCommands&);
public:
   CursorAndFocusCommands(AudacityProject *project);
   void Create(CommandManager *c);
   void CreateNonMenuCommands(CommandManager *c);

public:
   void OnSelectAll();
   // This is not bound to a menu item:
   void SelectAllIfNone();
   void OnSelectNone();

private:
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   void OnToggleSpectralSelection();
   void DoNextPeakFrequency(bool up);
   void OnNextHigherPeakFrequency();
   void OnNextLowerPeakFrequency();
#endif

   void OnSetLeftSelection();
   void OnSetRightSelection();
   void OnSelectStartCursor();
   void OnSelectCursorEnd();
   void OnSelectAllTracks();
   void OnSelectSyncLockSel();

   void OnZeroCrossing();
   double NearestZeroCrossing(double t0);

   void OnCursorSelStart();
   void OnCursorSelEnd();
   void OnCursorTrackStart();
   void OnCursorTrackEnd();
   void OnSelectionSave();
   void OnSelectionRestore();

   // non-menu commands
   void PrevWindow();
   void NextWindow();

   void PrevFrame();
   void NextFrame();

   void OnSelToStart();
   void OnSelToEnd();

   // Enabled only during play/record:
   void OnSeekLeftShort();
   void OnSeekRightShort();
   void OnSeekLeftLong();
   void OnSeekRightLong();
   //

   void OnCursorUp();
   void OnCursorDown();
   void OnFirstTrack();
   void OnLastTrack();
   void OnShiftUp();
   void OnShiftDown();
   void OnPrevTrack(bool shift = false);
   void OnNextTrack(bool shift = false);
   void OnToggle();

   AudacityProject *mProject;
   SelectedRegion mRegionSave;
};

#endif
