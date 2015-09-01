#ifndef __AUDACITY_FILE_MENU_COMMANDS__
#define __AUDACITY_FILE_MENU_COMMANDS__

#include "../Audacity.h"

class AudacityProject;
class CommandManager;

class FileMenuCommands
{
   FileMenuCommands(const FileMenuCommands&);
   FileMenuCommands& operator= (const FileMenuCommands&);
public:
   FileMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);

private:
   void OnNew();
public:
   void OnOpen();
private:
   void OnClose();
   void OnSave();
   void OnSaveAs();
#ifdef USE_LIBVORBIS
   void OnSaveCompressed();
#endif
   void OnCheckDependencies();
   void OnEditMetadata();

   AudacityProject *const mProject;
};

#endif
