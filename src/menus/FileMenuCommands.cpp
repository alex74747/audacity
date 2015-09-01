#include "FileMenuCommands.h"

#include "../Project.h"
#include "../commands/CommandManager.h"

#define FN(X) new ObjectCommandFunctor<FileMenuCommands>(this, &FileMenuCommands:: X )

FileMenuCommands::FileMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void FileMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

   /*i18n-hint: "New" is an action (verb) to create a new project*/
   c->AddItem(wxT("New"), _("&New"), FN(OnNew), wxT("Ctrl+N"),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);
}

void FileMenuCommands::OnNew()
{
   ::CreateNewAudacityProject();
}
