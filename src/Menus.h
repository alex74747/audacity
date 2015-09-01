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

void PopulateEffectsMenu(CommandManager *c, EffectType type, int batchflags, int realflags);
void AddEffectMenuItems(CommandManager *c, EffectPlugs & plugs, int batchflags, int realflags, bool isDefault);
void AddEffectMenuItemGroup(CommandManager *c, const wxArrayString & names, const PluginIDList & plugs, const wxArrayInt & flags, bool isDefault);

public:
void CreateRecentFilesMenu(CommandManager *c);
void ModifyUndoMenuItems();

void ModifyToolbarMenus();
// Calls ModifyToolbarMenus() on all projects
void ModifyAllProjectToolbarMenus();

int GetFocusedFrame();

public:
wxUint32 GetUpdateFlags();

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

        // File Menu

void OnClose();

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
void OnImportRaw();

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

// Make sure we return to "public" for subsequent declarations in Project.h.
public:

#endif



