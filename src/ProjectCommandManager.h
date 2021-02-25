/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectCommandManager.h

  Paul Licameli split from CommandManager.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_COMMAND_MANAGER__
#define __AUDACITY_PROJECT_COMMAND_MANAGER__

class ProjectCommandManager final {
public:
   static void UpdateCheckmarksInAllProjects();
};

#endif
