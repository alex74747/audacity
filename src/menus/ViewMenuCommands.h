#ifndef __AUDACITY_VIEW_MENU_COMMANDS__
#define __AUDACITY_VIEW_MENU_COMMANDS__

class AudacityProject;
class CommandManager;

class ViewMenuCommands
{
   ViewMenuCommands(const ViewMenuCommands&);
   ViewMenuCommands& operator= (const ViewMenuCommands&);
public:
   ViewMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);

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

   AudacityProject *const mProject;
};

#endif
