#include "TransportMenuCommands.h"

#define FN(X) new ObjectCommandFunctor<TransportMenuCommands>(this, &TransportMenuCommands:: X )

TransportMenuCommands::TransportMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void TransportMenuCommands::Create(CommandManager *c)
{
}
