#ifndef __AUDACITY_HELP_MENU_COMMANDS__
#define __AUDACITY_HELP_MENU_COMMANDS__

class AudacityProject;
class CommandManager;

class HelpMenuCommands
{
   HelpMenuCommands(const HelpMenuCommands&);
   HelpMenuCommands& operator= (const HelpMenuCommands&);
public:
   HelpMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);

private:

   AudacityProject *const mProject;
};

#endif
