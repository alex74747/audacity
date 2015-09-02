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

public:
void ModifyUndoMenuItems();

void ModifyToolbarMenus();
// Calls ModifyToolbarMenus() on all projects
void ModifyAllProjectToolbarMenus();

CommandFlag GetFocusedFrame();

public:
// If checkActive, do not do complete flags testing on an
// inactive project as it is needlessly expensive.
CommandFlag GetUpdateFlags(bool checkActive = false);

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

        // Selection-Editing Commands

void OnSelContractLeft(const wxEvent * evt);
void OnSelContractRight(const wxEvent * evt);

void OnSelSetExtendLeft();
void OnSelSetExtendRight();


void OnSnapToOff();
void OnSnapToNearest();
void OnSnapToPrior();

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

void OnPageSetup();
void OnPrint();

void OnExit();

public:

        // View Menu

// void OnZoomToggle();
void DoZoomFitV();


void OnPlotSpectrum();
void OnContrast();


// Import Submenu
void OnImport();
void OnImportLabels();
void OnImportMIDI();
void DoImportMIDI(const wxString &fileName);
void OnImportRaw();

void OnEditMetadata();
bool DoEditMetadata(const wxString &title, const wxString &shortUndoDescription, bool force);

public:
bool CursorPositionHasBeenStored() const
{ return mCursorPositionHasBeenStored; }

void SetCursorPositionHasBeenStored(bool value)
{ mCursorPositionHasBeenStored = value; }

double CursorPositionStored() const
{ return mCursorPositionStored; }

void SetCursorPositionStored(double value)
{ mCursorPositionStored = value; }

private:
   SelectedRegion mRegionSave{};
   bool mCursorPositionHasBeenStored{false};
   double mCursorPositionStored;
public:

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

void OnCursorLeft(bool shift, bool ctrl, bool keyup = false);
void OnCursorRight(bool shift, bool ctrl, bool keyup = false);
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



