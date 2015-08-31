#include "../Audacity.h"
#include "EditMenuCommands.h"

#define FN(X) FNT(EditMenuCommands, this, & EditMenuCommands :: X)

EditMenuCommands::EditMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void EditMenuCommands::Create(CommandManager *c)
{
}
