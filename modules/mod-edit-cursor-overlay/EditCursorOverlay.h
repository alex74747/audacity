/**********************************************************************

Audacity: A Digital Audio Editor

EditCursorOverlay.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_EDIT_CURSOR_OVERLAY__
#define __AUDACITY_EDIT_CURSOR_OVERLAY__

#include <memory>
#include "Project.h"
#include "Overlay.h" // to inherit

class AudacityProject;

class EditCursorOverlay final
   : public Overlay
   , public AttachedProjectObject
{
public:
   explicit
   EditCursorOverlay(AudacityProject *project, bool isMaster = true);

private:
   unsigned SequenceNumber() const override;
   std::pair<wxRect, bool> DoGetRectangle(wxSize size) override;
   void Draw(OverlayPanel &panel, wxDC &dc) override;

   AudacityProject *mProject;
   bool mIsMaster;
   std::shared_ptr<EditCursorOverlay> mPartner;

   int mLastCursorX;
   double mCursorTime;
   int mNewCursorX;
};

#endif
