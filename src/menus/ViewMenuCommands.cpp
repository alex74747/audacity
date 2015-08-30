#include "../Audacity.h"
#include "ViewMenuCommands.h"

#include "../Project.h"
#include "../commands/CommandManager.h"

#define FN(X) FNT(ViewMenuCommands, this, & ViewMenuCommands :: X)

ViewMenuCommands::ViewMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void ViewMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(TracksExistFlag, TracksExistFlag);

   c->AddItem(wxT("ZoomIn"), _("Zoom &In"), FN(OnZoomIn), wxT("Ctrl+1"),
      ZoomInAvailableFlag,
      ZoomInAvailableFlag);
}

void ViewMenuCommands::OnZoomIn()
{
   mProject->ZoomInByFactor(2.0);
}
