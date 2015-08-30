#ifndef __AUDACITY_TRANSPORT_MENU_COMMANDS__
#define __AUDACITY_TRANSPORT_MENU_COMMANDS__

class AudacityProject;
class CommandManager;

class TransportMenuCommands
{
   TransportMenuCommands(const TransportMenuCommands&);
   TransportMenuCommands& operator= (const TransportMenuCommands&);
public:
   TransportMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);
   void CreateNonMenuCommands(CommandManager *c);

   void OnPlayStop();
private:

   AudacityProject *const mProject;
};

#endif
