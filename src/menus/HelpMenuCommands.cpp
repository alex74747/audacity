#include "HelpMenuCommands.h"

#include "../commands/CommandManager.h"
#define FN(X) new ObjectCommandFunctor<HelpMenuCommands>(this, &HelpMenuCommands:: X )

HelpMenuCommands::HelpMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void HelpMenuCommands::Create(CommandManager *c)
{
}
