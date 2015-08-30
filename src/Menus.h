/**********************************************************************

  Audacity: A Digital Audio Editor

  Menus.h

  Dominic Mazzoni

**********************************************************************/
#ifndef __AUDACITY_MENUS__
#define __AUDACITY_MENUS__

#include "Experimental.h"


// These are all member functions of class AudacityProject.
// Vaughan, 2010-08-05:
//    Note that this file is included in a "public" section of Project.h.
//    Most of these methods do not need to be public, and because
//    we do not subclass AudacityProject, they should be "private."
//    Because the ones that need to be public are intermixed,
//    I've added "private" in just a few cases.

private:
void CreateMenusAndCommands();

void PopulateEffectsMenu(CommandManager *c, EffectType type,
                         CommandFlag batchflags, CommandFlag realflags);
void AddEffectMenuItems(CommandManager *c, EffectPlugs & plugs,
                        CommandFlag batchflags, CommandFlag realflags, bool isDefault);
void AddEffectMenuItemGroup(CommandManager *c, const wxArrayString & names,
                            const PluginIDList & plugs,
                            const std::vector<CommandFlag> & flags, bool isDefault);
void CreateRecentFilesMenu(CommandManager *c);
void ModifyUndoMenuItems();
void ModifyToolbarMenus();

public:
// Calls ModifyToolbarMenus() on all projects
void ModifyAllProjectToolbarMenus();

private:
CommandFlag GetFocusedFrame();

// If checkActive, do not do complete flags testing on an
// inactive project as it is needlessly expensive.
CommandFlag GetUpdateFlags(bool checkActive = false);

double NearestZeroCrossing(double t0);

private:

        // Selecting a tool from the keyboard

void SetTool(int tool);
void OnSelectTool();
void OnZoomTool();
void OnEnvelopeTool();
void OnTimeShiftTool();
void OnDrawTool();
void OnMultiTool();

void OnNextTool();
void OnPrevTool();

public:
        // Audio I/O Commands

void OnStopSelect();
void OnSeekLeftShort();
void OnSeekRightShort();
void OnSeekLeftLong();
void OnSeekRightLong();

        // Moving track focus commands

void OnCursorUp();
void OnCursorDown();
void OnFirstTrack();
void OnLastTrack();

        // Selection-Editing Commands

void OnShiftUp();
void OnShiftDown();
void OnToggle();

void OnCursorLeft(const wxEvent * evt);
void OnCursorRight(const wxEvent * evt);
void OnSelExtendLeft(const wxEvent * evt);
void OnSelExtendRight(const wxEvent * evt);
void OnSelContractLeft(const wxEvent * evt);
void OnSelContractRight(const wxEvent * evt);

void OnCursorShortJumpLeft();
void OnCursorShortJumpRight();
void OnCursorLongJumpLeft();
void OnCursorLongJumpRight();
void OnSelSetExtendLeft();
void OnSelSetExtendRight();

void OnSetLeftSelection();
void OnSetRightSelection();

void OnSelToStart();
void OnSelToEnd();

void OnZeroCrossing();

void OnLockPlayRegion();
void OnUnlockPlayRegion();

void OnSnapToOff();
void OnSnapToNearest();
void OnSnapToPrior();
void OnFullScreen();

static void DoMacMinimize(AudacityProject *project);
void OnMacMinimize();
void OnMacMinimizeAll();
void OnMacZoom();
void OnMacBringAllToFront();

        // File Menu

void OnNew();
void OnOpen();
void OnClose();
void OnSave();
void OnSaveAs();
#ifdef USE_LIBVORBIS
   void OnSaveCompressed();
#endif

void OnCheckDependencies();

void OnExport();
void OnExportSelection();
void OnExportMultiple();
void OnExportLabels();
void OnExportMIDI();

void OnPreferences();

void OnPageSetup();
void OnPrint();

void OnExit();

        // Edit Menu

public:
void OnUndo();
void OnRedo();

void OnCut();
void OnSplitCut();
void OnCopy();

void OnPaste();
private:
bool HandlePasteText(); // Handle text paste (into active label), if any. Return true if pasted.
bool HandlePasteNothingSelected(); // Return true if nothing selected, regardless of paste result.
public:

void OnPasteNewLabel();
void OnPasteOver();
void OnTrim();

void OnDelete();
void OnSplitDelete();
void OnSilence();

void OnSplit();
void OnSplitNew();
void OnJoin();
void OnDisjoin();
void OnDuplicate();

void OnCutLabels();
void OnSplitCutLabels();
void OnCopyLabels();
void OnDeleteLabels();
void OnSplitDeleteLabels();
void OnSilenceLabels();
void OnSplitLabels();
void OnJoinLabels();
void OnDisjoinLabels();

void OnSelectAll();
void OnSelectNone();
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
void OnToggleSpectralSelection();
void DoNextPeakFrequency(bool up);
void OnNextHigherPeakFrequency();
void OnNextLowerPeakFrequency();
#endif
void OnSelectCursorEnd();
void OnSelectStartCursor();
void OnSelectCursorStoredCursor();
void OnSelectSyncLockSel();
void OnSelectAllTracks();

        // View Menu

// void OnZoomToggle();
void DoZoomFitV();

void OnExpandAllTracks();
void OnCollapseAllTracks();

void OnShowClipping();

void OnHistory();

void OnKaraoke();
void OnMixerBoard();

void OnPlotSpectrum();
void OnContrast();

void OnShowTransportToolBar();
void OnShowDeviceToolBar();
void OnShowEditToolBar();
void OnShowMeterToolBar();
void OnShowRecordMeterToolBar();
void OnShowPlayMeterToolBar();
void OnShowMixerToolBar();
void OnShowSelectionToolBar();
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
void OnShowSpectralSelectionToolBar();
#endif
void OnShowScrubbingToolBar();
void OnShowToolsToolBar();
void OnShowTranscriptionToolBar();
void OnResetToolBars();


// Import Submenu
void OnImport();
void OnImportLabels();
void OnImportMIDI();
void DoImportMIDI(const wxString &fileName);
void OnImportRaw();

void OnEditMetadata();
bool DoEditMetadata(const wxString &title, const wxString &shortUndoDescription, bool force);

private:
   SelectedRegion mRegionSave{};
   bool mCursorPositionHasBeenStored{false};
   double mCursorPositionStored;
public:
void OnSelectionSave();
void OnSelectionRestore();
void OnCursorPositionStore();

void OnCursorTrackStart();
void OnCursorTrackEnd();
void OnCursorSelStart();
void OnCursorSelEnd();

// Effect Menu

bool OnEffect(const PluginID & ID, int flags = OnEffectFlagsNone);
void OnRepeatLastEffect(int index);
void OnApplyChain();
void OnEditChains();
void OnManagePluginsMenu(EffectType Type);
void OnManageGenerators();
void OnManageEffects();
void OnManageAnalyzers();

        // Help Menu

void OnHelpWelcome();

       //

void OnSeparator();

      // Keyboard navigation

void NextOrPrevFrame(bool next);
void PrevFrame();
void NextFrame();

void PrevWindow();
void NextWindow();

private:
void OnCursorLeft(bool shift, bool ctrl, bool keyup = false);
void OnCursorRight(bool shift, bool ctrl, bool keyup = false);
void OnCursorMove(bool forward, bool jump, bool longjump);
void OnBoundaryMove(bool left, bool boundaryContract);

// Handle small cursor and play head movements
void SeekLeftOrRight
(bool left, bool shift, bool ctrl, bool keyup,
 int snapToTime, bool mayAccelerateQuiet, bool mayAccelerateAudio,
 double quietSeekStepPositive, bool quietStepIsPixels,
 double audioSeekStepPositive, bool audioStepIsPixels);

// Helper for moving by keyboard with snap-to-grid enabled
double GridMove(double t, int minPix);

// Make sure we return to "public" for subsequent declarations in Project.h.
public:

#endif



