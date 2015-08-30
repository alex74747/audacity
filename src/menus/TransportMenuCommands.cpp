#include "../Audacity.h"
#include "TransportMenuCommands.h"

#define FN(X) FNT(TransportMenuCommands, this, & TransportMenuCommands :: X)

TransportMenuCommands::TransportMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void TransportMenuCommands::Create(CommandManager *c)
{
}
