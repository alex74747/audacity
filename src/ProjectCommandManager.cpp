/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectCommandManager.cpp

  Paul Licameli split from CommandManager.cpp

**********************************************************************/

#include "ProjectCommandManager.h"
#include "commands/CommandManager.h"
#include "Project.h"

void ProjectCommandManager::UpdateCheckmarksInAllProjects()
{
   for (auto pProject : AllProjects{}) {
      auto &project = *pProject;
      CommandManager::Get(project).UpdateCheckmarks(project);
   }
}
