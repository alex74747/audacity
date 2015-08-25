/**********************************************************************

Audacity: A Digital Audio Editor

PlayIndicatorOverlay.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_PLAY_INDICATOR_OVERLAY__
#define __AUDACITY_PLAY_INDICATOR_OVERLAY__

#include "../../TrackPanelOverlay.h"
#include <wx/event.h>

class AudacityProject;


class PlayIndicatorOverlay : public wxEvtHandler, public TrackPanelOverlay
{
public:
   PlayIndicatorOverlay(AudacityProject *project);
   virtual ~PlayIndicatorOverlay();

private:
   virtual std::pair<wxRect, bool> GetRectangle(wxSize size);
   virtual void Draw
      (wxDC &dc, TrackPanelCellIterator begin, TrackPanelCellIterator end);

   void OnTimer(wxCommandEvent &event);

   AudacityProject *mProject;
   int mLastIndicatorX;
   int mNewIndicatorX;
};

#endif
