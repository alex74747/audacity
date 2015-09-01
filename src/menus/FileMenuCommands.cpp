#include "FileMenuCommands.h"

#define FN(X) new ObjectCommandFunctor<FileMenuCommands>(this, &FileMenuCommands:: X )

FileMenuCommands::FileMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void FileMenuCommands::Create(CommandManager *c)
{
}
