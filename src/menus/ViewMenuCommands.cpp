#include "../Audacity.h"
#include "ViewMenuCommands.h"

#include <algorithm>

#include <wx/scrolbar.h>

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
   c->AddItem(wxT("ZoomSel"), _("&Zoom to Selection"), FN(OnZoomSel), wxT("Ctrl+E"), TimeSelectedFlag, TimeSelectedFlag);

   c->AddSeparator();
   c->AddItem(wxT("FitInWindow"), _("&Fit in Window"), FN(OnZoomFit), wxT("Ctrl+F"));
   c->AddItem(wxT("FitV"), _("Fit &Vertically"), FN(OnZoomFitV), wxT("Ctrl+Shift+F"));

   c->AddSeparator();
   c->AddItem(wxT("GoSelStart"), _("Go to Selection Sta&rt"), FN(OnGoSelStart), wxT("Ctrl+["), TimeSelectedFlag, TimeSelectedFlag);
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

void ViewMenuCommands::OnZoomSel()
{
   const double lowerBound =
      std::max(mProject->GetViewInfo().selectedRegion.t0(), mProject->ScrollingLowerBoundTime());
   const double denom =
      mProject->GetViewInfo().selectedRegion.t1() - lowerBound;
   if (denom <= 0.0)
      return;

   // LL:  The "-1" is just a hack to get around an issue where zooming to
   //      selection doesn't actually get the entire selected region within the
   //      visible area.  This causes a problem with scrolling at end of playback
   //      where the selected region may be scrolled off the left of the screen.
   //      I know this isn't right, but until the real rounding or 1-off issue is
   //      found, this will have to work.
   // PRL:  Did I fix this?  I am not sure, so I leave the hack in place.
   //      Fixes might have resulted from commits
   //      1b8f44d0537d987c59653b11ed75a842b48896ea and
   //      e7c7bb84a966c3b3cc4b3a9717d5f247f25e7296
   int width;
   mProject->GetTrackPanel()->GetTracksUsableArea(&width, NULL);
   mProject->Zoom((width - 1) / denom);
   mProject->TP_ScrollWindow(mProject->GetViewInfo().selectedRegion.t0());
}

void ViewMenuCommands::OnZoomFit()
{
   const double end = mProject->GetTracks()->GetEndTime();
   const double start = mProject->GetViewInfo().bScrollBeyondZero
      ? std::min(mProject->GetTracks()->GetStartTime(), 0.0)
      : 0;
   const double len = end - start;

   if (len <= 0.0)
      return;

   int w;
   mProject->GetTrackPanel()->GetTracksUsableArea(&w, NULL);
   w -= 10;

   mProject->Zoom(w / len);
   mProject->TP_ScrollWindow(start);
}

void ViewMenuCommands::OnZoomFitV()
{
   mProject->DoZoomFitV();

   mProject->GetVerticalScrollBar()->SetThumbPosition(0);
   mProject->RedrawProject();
   mProject->ModifyState(true);
}

void ViewMenuCommands::OnGoSelStart()
{
   const ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   mProject->TP_ScrollWindow(viewInfo.selectedRegion.t0() -
      ((mProject->GetScreenEndTime() - viewInfo.h) / 2));
}
