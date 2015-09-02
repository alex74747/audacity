#ifndef __AUDACITY_CURSOR_AND_FOCUS_COMMANDS__
#define __AUDACITY_CURSOR_AND_FOCUS_COMMANDS__

#include <wx/longlong.h>

#include "../SelectedRegion.h"

class AudacityProject;
class CommandManager;

class wxEvent;

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
   void OnCursorLeft(const wxEvent * evt);
   void OnCursorRight(const wxEvent * evt);

   void OnCursorShortJumpLeft();
   void OnCursorShortJumpRight();
   void OnCursorLongJumpLeft();
   void OnCursorLongJumpRight();
   void OnCursorMove(bool forward, bool jump, bool longjump);

   void OnSelExtendLeft(const wxEvent * evt);
   void OnSelExtendRight(const wxEvent * evt);

   void OnSelSetExtendLeft();
   void OnSelSetExtendRight();
   void OnBoundaryMove(bool left, bool boundaryContract);

   void OnSelContractLeft(const wxEvent * evt);
   void OnCursorRight(bool shift, bool ctrl, bool keyup = false);

   void OnSelContractRight(const wxEvent * evt);
   void OnCursorLeft(bool shift, bool ctrl, bool keyup = false);

   // Handle small cursor and play head movements
   void SeekLeftOrRight
      (bool left, bool shift, bool ctrl, bool keyup,
      int snapToTime, bool mayAccelerateQuiet, bool mayAccelerateAudio,
      double quietSeekStepPositive, bool quietStepIsPixels,
      double audioSeekStepPositive, bool audioStepIsPixels);
   // Helper for moving by keyboard with snap-to-grid enabled
   double GridMove(double t, int minPix);

   void OnSnapToOff();

   AudacityProject *mProject;
   SelectedRegion mRegionSave;
   wxLongLong mLastSelectionAdjustment;
};

#endif
