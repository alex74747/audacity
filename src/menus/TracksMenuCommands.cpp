#include "../Audacity.h"
#include "TracksMenuCommands.h"

#define FN(X) FNT(TracksMenuCommands, this, & TracksMenuCommands :: X)

TracksMenuCommands::TracksMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void TracksMenuCommands::Create(CommandManager *c)
{
}
