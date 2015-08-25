/**********************************************************************

Audacity: A Digital Audio Editor

TrackPanelOverlay.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "TrackPanelOverlay.h"

#include <wx/dc.h>

TrackPanelOverlay::~TrackPanelOverlay()
{
}

void TrackPanelOverlay::Erase(wxDC &dc, wxDC &src)
{
   wxRect rect(dc.GetSize());
   rect.Intersect(src.GetSize());
   rect.Intersect(GetRectangle(src.GetSize()).first);
   if (!rect.IsEmpty())
      dc.Blit(rect.x, rect.y, rect.width, rect.height,
              &src, rect.x, rect.y);
}
