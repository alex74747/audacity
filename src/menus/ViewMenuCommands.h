#ifndef __AUDACITY_VIEW_MENU_COMMANDS__
#define __AUDACITY_VIEW_MENU_COMMANDS__

#include "../Experimental.h"

class AudacityProject;
class CommandManager;

class ViewMenuCommands
{
   ViewMenuCommands(const ViewMenuCommands&);
   ViewMenuCommands& operator= (const ViewMenuCommands&);
public:
   ViewMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);
   void CreateNonMenuCommands(CommandManager *c);

   void OnZoomIn();
private:
   void OnZoomNormal();
public:
   void OnZoomOut();
   void OnZoomSel();
   void OnZoomFit();
private:
   void OnZoomFitV();
   void OnGoSelStart();
   void OnGoSelEnd();
   void OnCollapseAllTracks();
   void OnExpandAllTracks();
   void OnShowClipping();
   void OnHistory();
   void OnKaraoke();
   void OnMixerBoard();
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
   void OnShowToolsToolBar();
   void OnShowTranscriptionToolBar();
   void OnShowTransportToolBar();
   void OnResetToolBars();

   // non-menu commands
   void OnFullScreen();

   AudacityProject *const mProject;
};

#endif
