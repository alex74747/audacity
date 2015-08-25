/**********************************************************************

Audacity: A Digital Audio Editor

EditCursorOverlay.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_EDIT_CURSOR_OVERLAY__
#define __AUDACITY_EDIT_CURSOR_OVERLAY__

#include "../../TrackPanelOverlay.h"

class AudacityProject;

class EditCursorOverlay : public TrackPanelOverlay
{
public:
   EditCursorOverlay(AudacityProject *project);
   virtual ~EditCursorOverlay();

private:
   virtual std::pair<wxRect, bool> GetRectangle(wxSize size);
   virtual void Draw
      (wxDC &dc, TrackPanelCellIterator begin, TrackPanelCellIterator end);

   AudacityProject *mProject;

   int mLastCursorX;
   double mCursorTime;
   int mNewCursorX;
};

#endif
