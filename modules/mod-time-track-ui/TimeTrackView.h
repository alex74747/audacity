/**********************************************************************

Audacity: A Digital Audio Editor

TimeTrackView.h

Paul Licameli split from class TimeTrack

**********************************************************************/

#ifndef __AUDACITY_TIME_TRACK_VIEW__
#define __AUDACITY_TIME_TRACK_VIEW__

#include "CommonTrackView.h"

class EnvelopeHandle;
class Ruler;
class ZoomInfo;

class TimeTrackView final : public CommonTrackView
{
   TimeTrackView( const TimeTrackView& ) = delete;
   TimeTrackView &operator=( const TimeTrackView& ) = delete;

public:
   explicit
   TimeTrackView(
      const std::shared_ptr<Track> &pTrack, const ZoomInfo &zoomInfo );
   ~TimeTrackView() override;

private:
   Ruler &GetRuler() const { return *mRuler; }

   std::vector<UIHandlePtr> DetailedHitTest
      (const TrackPanelMouseState &state,
       const AudacityProject *pProject, int currentTool, bool bMultiTool)
      override;

   std::weak_ptr<EnvelopeHandle> mEnvelopeHandle;

   // TrackPanelDrawable implementation
   void Draw(
      TrackPanelDrawingContext &context,
      const wxRect &rect, unsigned iPass ) override;

   std::unique_ptr<Ruler> mRuler;
};

#endif
