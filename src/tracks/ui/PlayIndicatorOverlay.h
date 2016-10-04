/**********************************************************************

Audacity: A Digital Audio Editor

PlayIndicatorOverlay.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_PLAY_INDICATOR_OVERLAY__
#define __AUDACITY_PLAY_INDICATOR_OVERLAY__

#include <wx/event.h>
#include "../../MemoryX.h"
#include "../../widgets/Overlay.h"

class AudacityProject;


// Common class for overlaying track panel or ruler
class PlayIndicatorOverlayBase : public wxEvtHandler, public Overlay
{
public:
   PlayIndicatorOverlayBase(AudacityProject *project, bool isMaster);
   virtual ~PlayIndicatorOverlayBase();

   void Update(wxInt64 newIndicatorX) { mNewIndicatorX = newIndicatorX; }

private:
   std::pair<wxRect, bool> DoGetRectangle(wxSize size) override;
   void Draw(OverlayPanel &panel, wxDC &dc) override;

protected:

   AudacityProject *const mProject;
   const bool mIsMaster;
   wxInt64 mLastIndicatorX { -1 };
   wxInt64 mNewIndicatorX { -1 };
};

// Master object for track panel, creates the other object for the ruler
class PlayIndicatorOverlay final : public PlayIndicatorOverlayBase
{
public:
   PlayIndicatorOverlay(AudacityProject *project);
   virtual ~PlayIndicatorOverlay();

private:
   void OnTimer(wxCommandEvent &event);

   std::unique_ptr<PlayIndicatorOverlayBase> mPartner;
};

#endif
