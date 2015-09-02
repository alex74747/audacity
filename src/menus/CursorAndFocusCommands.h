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
   void OnSelectCursorStoredCursor();
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
   void OnCursorPositionStore();

   // non-menu commands
   void PrevWindow();
   void NextWindow();

public:
   void NextOrPrevFrame(bool forward);

private:
   void PrevFrame();
   void NextFrame();

   AudacityProject *mProject;
   SelectedRegion mRegionSave;
};

#endif
