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

   AudacityProject *const mProject;
};

#endif
