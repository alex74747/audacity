#include "EditMenuCommands.h"

#define FN(X) new ObjectCommandFunctor<EditMenuCommands>(this, &EditMenuCommands:: X )

EditMenuCommands::EditMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void EditMenuCommands::Create(CommandManager *c)
{
}
