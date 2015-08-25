/**********************************************************************

Audacity: A Digital Audio Editor

Scrubbing.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_SCRUBBING__
#define __AUDACITY_SCRUBBING__

#include "../../TrackPanelOverlay.h"

class AudacityProject;

class ScrubbingOverlay : public wxEvtHandler, public TrackPanelOverlay
{
public:
   ScrubbingOverlay(AudacityProject *project);
   virtual ~ScrubbingOverlay();

private:
   virtual std::pair<wxRect, bool> GetRectangle(wxSize size);
   virtual void Draw
      (wxDC &dc, TrackPanelCellIterator begin, TrackPanelCellIterator end);

   void OnTimer(wxCommandEvent &event);

   AudacityProject *mProject;
};

#endif
