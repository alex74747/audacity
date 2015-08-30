#include "TracksMenuCommands.h"

#define FN(X) new ObjectCommandFunctor<TracksMenuCommands>(this, &TracksMenuCommands:: X )

TracksMenuCommands::TracksMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void TracksMenuCommands::Create(CommandManager *c)
{
}
