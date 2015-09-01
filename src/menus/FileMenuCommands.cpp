#include "FileMenuCommands.h"

#define FN(X) FNT(FileMenuCommands, this, & FileMenuCommands :: X)

FileMenuCommands::FileMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void FileMenuCommands::Create(CommandManager *c)
{
}
