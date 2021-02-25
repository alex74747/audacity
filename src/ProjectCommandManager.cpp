/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectCommandManager.cpp

  Paul Licameli split from CommandManager.cpp

**********************************************************************/

#include "ProjectCommandManager.h"
#include "commands/CommandManager.h"
#include "ActiveProject.h"
#include "JournalRegistry.h"
#include "Menus.h"
#include "Project.h"
#include "commands/CommandContext.h"
#include "commands/CommandTargets.h"

void ProjectCommandManager::UpdateCheckmarksInAllProjects()
{
   for (auto pProject : AllProjects{}) {
      auto &project = *pProject;
      CommandManager::Get(project).UpdateCheckmarks(project);
   }
}

namespace {

constexpr auto JournalCode = wxT("CM");  // for CommandManager

// Register a callback for the journal
Journal::RegisteredCommand sCommand{ JournalCode,
[]( const wxArrayString &fields )
{
   // Expect JournalCode and the command name.
   // To do, perhaps, is to include some parameters.
   bool handled = false;
   if ( fields.size() == 2 ) {
      if (auto project = GetActiveProject().lock()) {
         auto pManager = &CommandManager::Get( *project );
         auto flags = MenuManager::Get( *project ).GetUpdateFlags();
         const CommandContext context( *project );
         auto &command = fields[1];
         handled =
            pManager->HandleTextualCommand( command, context, flags, false );
      }
   }
   return handled;
}
};

}
