/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectCommandManager.cpp

  Paul Licameli split from CommandManager.cpp

\class InteractiveOutputTargets
\brief InteractiveOutputTargets is an output target that pops up a
dialog, if necessary.

\class MessageDialogTarget
\brief MessageDialogTarget is a CommandOutputTarget that sends its status
to the LongMessageDialog.

\class LongMessageDialog
\brief LongMessageDialog is a dialog with a Text Window in it to
capture the more lengthy output from some commands.

**********************************************************************/

#include "ProjectCommandManager.h"
#include "commands/CommandManager.h"
#include "ActiveProject.h"
#include "DefaultCommandOutputTargets.h"
#include "JournalRegistry.h"
#include "Menus.h"
#include "Project.h"
#include "commands/CommandContext.h"
#include "Project.h"
#include "ShuttleGui.h"
#include "commands/CommandTargets.h"
#include "wxPanelWrapper.h"

#include <wx/app.h>
#include <wx/statusbr.h>

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

void StatusBarTarget::Update(const wxString &message)
{
   mStatus.SetStatusText(message, 0);
}
