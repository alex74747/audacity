#include "../Audacity.h"
#include "ViewMenuCommands.h"

#include "../Project.h"
#include "../TrackPanel.h"
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
   c->AddItem(wxT("ZoomNormal"), _("Zoom &Normal"), FN(OnZoomNormal), wxT("Ctrl+2"));
   c->AddItem(wxT("ZoomOut"), _("Zoom &Out"), FN(OnZoomOut), wxT("Ctrl+3"),
      ZoomOutAvailableFlag,
      ZoomOutAvailableFlag);
}

void ViewMenuCommands::OnZoomIn()
{
   mProject->ZoomInByFactor(2.0);
}

void ViewMenuCommands::OnZoomNormal()
{
   mProject->Zoom(ZoomInfo::GetDefaultZoom());
   mProject->GetTrackPanel()->Refresh(false);
}

void ViewMenuCommands::OnZoomOut()
{
   mProject->ZoomOutByFactor(1 / 2.0);
}
