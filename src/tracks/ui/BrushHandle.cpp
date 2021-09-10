/**********************************************************************

Audacity: A Digital Audio Editor

BrushHandle.cpp

Edward Hui

**********************************************************************/


#include "BrushHandle.h"
#include "Scrubbing.h"
#include "TrackView.h"

#include "../../AColor.h"
#include "../../SpectrumAnalyst.h"
#include "../../NumberScale.h"
#include "../../Project.h"
#include "../../ProjectAudioIO.h"
#include "../../ProjectHistory.h"
#include "../../ProjectSettings.h"
#include "../../ProjectWindow.h"
#include "../../RefreshCode.h"
#include "../../SelectUtilities.h"
#include "../../SelectionState.h"
#include "../../TrackArtist.h"
#include "../../TrackPanelAx.h"
#include "../../TrackPanel.h"
#include "../../TrackPanelDrawingContext.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../ViewInfo.h"
#include "../../WaveClip.h"
#include "../../WaveTrack.h"
#include "../../prefs/SpectrogramSettings.h"
#include "../../../images/Cursors.h"
#include "../playabletrack/wavetrack/ui/SpectrumView.h"

#include <tgmath.h>
#include <wx/event.h>

// Only for definition of SonifyBeginModifyState:
//#include "../../NoteTrack.h"

enum {
   //This constant determines the size of the horizontal region (in pixels) around
   //the right and left selection bounds that can be used for horizontal selection adjusting
   //(or, vertical distance around top and bottom bounds in spectrograms,
   // for vertical selection adjusting)
   SELECTION_RESIZE_REGION = 3,

   // Seems 4 is too small to work at the top.  Why?
   FREQ_SNAP_DISTANCE = 10,
};

// #define SPECTRAL_EDITING_ESC_KEY

bool BrushHandle::IsClicked() const
{
   return mSelectionStateChanger.get() != NULL;
}

namespace
{
   /// Converts a frequency to screen y position.
   wxInt64 FrequencyToPosition(const WaveTrack *wt,
                               double frequency,
                               wxInt64 trackTopEdge,
                               int trackHeight)
   {
      const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
      float minFreq, maxFreq;
      wt->GetSpectrumBounds(&minFreq, &maxFreq);
      const NumberScale numberScale(settings.GetScale(minFreq, maxFreq));
      const float p = numberScale.ValueToPosition(frequency);
      return trackTopEdge + wxInt64((1.0 - p) * trackHeight);
   }

   /// Converts a position (mouse Y coordinate) to
   /// frequency, in Hz.
   double PositionToFrequency(const WaveTrack *wt,
                              bool maySnap,
                              wxInt64 mouseYCoordinate,
                              wxInt64 trackTopEdge,
                              int trackHeight)
   {
      const double rate = wt->GetRate();

      // Handle snapping
      if (maySnap &&
          mouseYCoordinate - trackTopEdge < FREQ_SNAP_DISTANCE)
         return rate;
      if (maySnap &&
          trackTopEdge + trackHeight - mouseYCoordinate < FREQ_SNAP_DISTANCE)
         return -1;

      const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
      float minFreq, maxFreq;
      wt->GetSpectrumBounds(&minFreq, &maxFreq);
      const NumberScale numberScale(settings.GetScale(minFreq, maxFreq));
      const double p = double(mouseYCoordinate - trackTopEdge) / trackHeight;
      return numberScale.PositionToValue(1.0 - p);
   }

   long long PositionToLongSample(const WaveTrack *wt,
                                  const ViewInfo &viewInfo,
                                  int trackTopEdgeX,
                                  int mousePosX)
   {
      wxInt64 posTime = viewInfo.PositionToTime(mousePosX, trackTopEdgeX);
      sampleCount sc = wt->TimeToLongSamples(posTime);
      return sc.as_long_long();
   }

   template<typename T>
   inline void SetIfNotNull(T * pValue, const T Value)
   {
      if (pValue == NULL)
         return;
      *pValue = Value;
   }

   // This returns true if we're a spectral editing track.
   inline bool isSpectralSelectionView(const TrackView *pTrackView) {
      return
            pTrackView &&
            pTrackView->IsSpectral() &&
            pTrackView->FindTrack() &&
            pTrackView->FindTrack()->TypeSwitch< bool >(
                  [&](const WaveTrack *wt) {
                     const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
                     return settings.SpectralSelectionEnabled();
                  });
   }
   wxCursor *CrosshairCursor()
   {
      static auto crosshairCursor =
            ::MakeCursor(wxCURSOR_IBEAM, CrosshairCursorXpm, 16, 16);
      return &*crosshairCursor;
   }
}

UIHandlePtr BrushHandle::HitTest
      (std::weak_ptr<BrushHandle> &holder,
       const TrackPanelMouseState &st, const AudacityProject *pProject,
       const std::shared_ptr<TrackView> &pTrackView,
       const std::shared_ptr<SpectralData> &mpData)
{
   // This handle is a little special because there may be some state to
   // preserve during movement before the click.
   auto old = holder.lock();
   bool oldUseSnap = true;
   if (old) {
      // It should not have started listening to timer events
      if( old->mTimerHandler ) {
         wxASSERT(false);
         // Handle this eventuality anyway, don't leave a dangling back-pointer
         // in the attached event handler.
         old->mTimerHandler.reset();
      }
      oldUseSnap = old->mUseSnap;
   }

   const auto &viewInfo = ViewInfo::Get( *pProject );
   auto result = std::make_shared<BrushHandle>(
         pTrackView, oldUseSnap, TrackList::Get( *pProject ),
         st, viewInfo, mpData, projectSettings.GetBrushRadius()
   );

   result = AssignUIHandlePtr(holder, result);

   //Make sure we are within the selected track
   // Adjusting the selection edges can be turned off in
   // the preferences...
   auto pTrack = pTrackView->FindTrack();
   if (!pTrack->GetSelected() || !viewInfo.bAdjustSelectionEdges)
   {
      return result;
   }

   {
      const wxRect &rect = st.rect;
      wxInt64 leftSel = viewInfo.TimeToPosition(viewInfo.selectedRegion.t0(), rect.x);
      wxInt64 rightSel = viewInfo.TimeToPosition(viewInfo.selectedRegion.t1(), rect.x);
      // Something is wrong if right edge comes before left edge
      wxASSERT(!(rightSel < leftSel));
      static_cast<void>(leftSel); // Suppress unused variable warnings if not in debug-mode
      static_cast<void>(rightSel);
   }

   return result;
}

UIHandle::Result BrushHandle::NeedChangeHighlight
      (const BrushHandle &oldState, const BrushHandle &newState)
{
   auto useSnap = oldState.mUseSnap;
   // This is guaranteed when constructing the NEW handle:
   wxASSERT( useSnap == newState.mUseSnap );
   if (!useSnap)
      return 0;

   auto &oldSnapState = oldState.mSnapStart;
   auto &newSnapState = newState.mSnapStart;
   if ( oldSnapState.Snapped() == newSnapState.Snapped() &&
        (!oldSnapState.Snapped() ||
         oldSnapState.outCoord == newSnapState.outCoord) )
      return 0;

   return RefreshCode::RefreshAll;
}

BrushHandle::BrushHandle
( const std::shared_ptr<TrackView> &pTrackView, bool useSnap,
  const TrackList &trackList,
  const TrackPanelMouseState &st, const ViewInfo &viewInfo,
  const std::shared_ptr<SpectralData> &mpSpectralData)
   : mpView{ pTrackView }
   , mpSpectralData(mpSpectralData)
   , mSnapManager{ std::make_shared<SnapManager>(
      *trackList.GetOwner(), trackList, viewInfo) }
{
   const wxMouseState &state = st.state;
   auto pTrack = pTrackView->FindTrack().get();
   auto wt = dynamic_cast<WaveTrack *>(pTrack);
   double rate = mpSpectralData->GetSR();

   mRect = st.rect;
   mBrushRadius = brushRadius;
   mFreqUpperBound = wt->GetSpectrogramSettings().maxFreq - 1;
   mFreqLowerBound = wt->GetSpectrogramSettings().minFreq + 1;
   // Borrowed from TimeToLongSample
   mSampleCountLowerBound = floor( pTrack->GetStartTime() * rate + 0.5);
   mSampleCountUpperBound = floor( pTrack->GetEndTime() * rate + 0.5);
}

BrushHandle::~BrushHandle()
{
}

namespace {
   // Is the distance between A and B less than D?
   template < class A, class B, class DIST > bool within(A a, B b, DIST d)
   {
      return (a > b - d) && (a < b + d);
   }

   inline double findMaxRatio(double center, double rate)
   {
      const double minFrequency = 1.0;
      const double maxFrequency = (rate / 2.0);
      const double frequency =
            std::min(maxFrequency,
                     std::max(minFrequency, center));
      return
            std::min(frequency / minFrequency, maxFrequency / frequency);
   }
}

void BrushHandle::Enter(bool, AudacityProject *project)
{

}

bool BrushHandle::Escape(AudacityProject *project)
{
   return false;
}

// Add or remove data accroding to the ctrl key press
void BrushHandle::HandleTimeFreqData(long long ll_sc, wxInt64 freq) {
   // Ignore the mouse dragging outside valid area
   // We should also check for the available freq. range that is visible to user
   if(ll_sc < mSampleCountLowerBound || ll_sc > mSampleCountUpperBound ||
      freq < mFreqLowerBound || freq > mFreqUpperBound)
      return;

   if(mbCtrlDown)
      mpSpectralData->removeTimeFreqData(ll_sc, freq);
   else
      mpSpectralData->addTimeFreqData(ll_sc, freq);
}

UIHandle::Result BrushHandle::Click
      (const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;

   const auto pView = mpView.lock();
   if ( !pView )
      return Cancelled;

   wxMouseEvent &event = evt.event;
   const auto sTrack = TrackList::Get( *pProject ).Lock( FindTrack() );
   const auto pTrack = sTrack.get();
   const WaveTrack *const wt =
         static_cast<const WaveTrack*>(pTrack);
   auto &trackPanel = TrackPanel::Get( *pProject );
   auto &viewInfo = ViewInfo::Get( *pProject );

   mMostRecentX = event.m_x;
   mMostRecentY = event.m_y;

   return RefreshAll;
}

UIHandle::Result BrushHandle::Drag
      (const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;

   const auto pView = mpView.lock();
   if ( !pView )
      return Cancelled;

   wxMouseEvent &event = evt.event;
   const auto sTrack = TrackList::Get( *pProject ).Lock( FindTrack() );
   const auto pTrack = sTrack.get();
   const WaveTrack *const wt =
         static_cast<const WaveTrack*>(pTrack);
   auto &trackPanel = TrackPanel::Get( *pProject );
   auto &viewInfo = ViewInfo::Get( *pProject );

   int x = mAutoScrolling ? mMostRecentX : event.m_x;
   int y = mAutoScrolling ? mMostRecentY : event.m_y;
   mMostRecentX = x;
   mMostRecentY = y;

   // Clip the coordinates
   // TODO: Find ways to access the ClipParameters (for the mid)
   int x1 = std::max(mRect.x + 10, std::min(x, mRect.x + mRect.width - 20));
   int y1 = std::max(mRect.y + 10, std::min(y, mRect.y + mRect.height - 10));

   mbCtrlDown = event.ControlDown();

   if(!mpSpectralData->coordHistory.empty()){
      int x0 = mpSpectralData->coordHistory.back().first;
      int y0 = mpSpectralData->coordHistory.back().second;
      int wd = 10;

      int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
      int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
      int err = dx+dy, err2;

      double posTime;
      wxInt64 posFreq;

      auto posToLongLong = [&](int x0){
         posTime = viewInfo.PositionToTime(x0, mRect.x);
         sampleCount sc = wt->TimeToLongSamples(posTime);
         return sc.as_long_long();
      };

      // Line drawing (draw points until the start coordinate reaches the end)
      while(true){
         if (x0 == x1 && y0 == y1)
            break;
         err2 = err * 2;
         if (err2 * 2 >= dy) { err += dy; x0 += sx; }
         if (err2 * 2 <= dx) { err += dx; y0 += sy; }

         // For each (x0, y0), draw circle based on Quadrant
         int xm = x0, ym = y0;
         int r = mBrushRadius;
         int cx = -r, cy = 0, cerr = 2 - 2 * r;
         do {
            posFreq = PositionToFrequency(wt, 0, ym+cy, mRect.y, mRect.height);
            HandleTimeFreqData(posToLongLong(xm-cx), posFreq);

            posFreq = PositionToFrequency(wt, 0, ym-cx, mRect.y, mRect.height);
            HandleTimeFreqData(posToLongLong(xm-cy), posFreq);

            posFreq = PositionToFrequency(wt, 0, ym-cy, mRect.y, mRect.height);
            HandleTimeFreqData(posToLongLong(xm+cx), posFreq);

            posFreq = PositionToFrequency(wt, 0, ym+cx, mRect.y, mRect.height);
            HandleTimeFreqData(posToLongLong(xm+cy), posFreq);

            r = cerr;
            if (r <= cy)
               cerr += ++cy * 2 + 1;
            if (r > cx || cerr > cy)
               cerr += ++cx * 2 + 1;
         } while (cx < 0); // End of circle drawing
      } // End of line connecting
   }
   mpSpectralData->coordHistory.push_back(std::make_pair(x, y));
   return RefreshAll;
}

HitTestPreview BrushHandle::Preview
      (const TrackPanelMouseState &st, AudacityProject *pProject)
{
   TranslatableString tip;
   wxCursor *pCursor = CrosshairCursor();
   return { tip, pCursor };
}

UIHandle::Result BrushHandle::Release
      (const TrackPanelMouseEvent &evt, AudacityProject *pProject,
       wxWindow *)
{
   using namespace RefreshCode;
   mpSpectralData->saveAndClearBuffer();
   if(mbCtrlDown){
      ProjectHistory::Get( *pProject ).PushState(
            XO( "Erased selected area" ),
            XO( "Erased selected area" ) );
      ProjectHistory::Get( *pProject ).ModifyState(true);
   }
   else{
      ProjectHistory::Get( *pProject ).PushState(
            XO( "Selected area using Brush Tool" ),
            XO( "Brush tool selection" ) );
      ProjectHistory::Get( *pProject ).ModifyState(true);
   }

   return RefreshNone;
}

UIHandle::Result BrushHandle::Cancel(AudacityProject *pProject)
{
   mSelectionStateChanger.reset();
   mpSpectralData->dataBuffer.clear();
   mpSpectralData->coordHistory.clear();

   return RefreshCode::RefreshAll;
}

void BrushHandle::Draw(
      TrackPanelDrawingContext &context,
      const wxRect &rect, unsigned iPass )
{
   if ( iPass == TrackArtist::PassTracks ) {
      auto& dc = context.dc;
   }
}

std::weak_ptr<Track> BrushHandle::FindTrack()
{
   auto pView = mpView.lock();
   if (!pView)
      return {};
   else
      return pView->FindTrack();
}

void BrushHandle::Connect(AudacityProject *pProject)
{
   mTimerHandler = std::make_shared<TimerHandler>( this, pProject );
}

class BrushHandle::TimerHandler : public wxEvtHandler
{
public:
   TimerHandler( BrushHandle *pParent, AudacityProject *pProject )
         : mParent{ pParent }
         , mConnectedProject{ pProject }
   {
      if (mConnectedProject)
         mConnectedProject->Bind(EVT_TRACK_PANEL_TIMER,
                                 &BrushHandle::TimerHandler::OnTimer,
                                 this);
   }

   // Receives timer event notifications, to implement auto-scroll
   void OnTimer(wxCommandEvent &event);

private:
   BrushHandle *mParent;
   AudacityProject *mConnectedProject;
};

void BrushHandle::TimerHandler::OnTimer(wxCommandEvent &event)
{
   event.Skip();

   // AS: If the user is dragging the mouse and there is a track that
   //  has captured the mouse, then scroll the screen, as necessary.

   ///  We check on each timer tick to see if we need to scroll.

   // DM: If we're "autoscrolling" (which means that we're scrolling
   //  because the user dragged from inside to outside the window,
   //  not because the user clicked in the scroll bar), then
   //  the selection code needs to be handled slightly differently.
   //  We set this flag ("mAutoScrolling") to tell the selecting
   //  code that we didn't get here as a result of a mouse event,
   //  and therefore it should ignore the event,
   //  and instead use the last known mouse position.  Setting
   //  this flag also causes the Mac to redraw immediately rather
   //  than waiting for the next update event; this makes scrolling
   //  smoother on MacOS 9.

   const auto project = mConnectedProject;
   const auto &trackPanel = TrackPanel::Get( *project );
   auto &window = ProjectWindow::Get( *project );
   if (mParent->mMostRecentX >= mParent->mRect.x + mParent->mRect.width) {
      mParent->mAutoScrolling = true;
      window.TP_ScrollRight();
   }
   else if (mParent->mMostRecentX < mParent->mRect.x) {
      mParent->mAutoScrolling = true;
      window.TP_ScrollLeft();
   }
   else {
      // Bug1387:  enable autoscroll during drag, if the pointer is at either
      // extreme x coordinate of the screen, even if that is still within the
      // track area.

      int xx = mParent->mMostRecentX, yy = 0;
      trackPanel.ClientToScreen(&xx, &yy);
      if (xx == 0) {
         mParent->mAutoScrolling = true;
         window.TP_ScrollLeft();
      }
      else {
         int width, height;
         ::wxDisplaySize(&width, &height);
         if (xx == width - 1) {
            mParent->mAutoScrolling = true;
            window.TP_ScrollRight();
         }
      }
   }

   auto pTrack = mParent->FindTrack().lock(); // TrackList::Lock() ?
   if (mParent->mAutoScrolling && pTrack) {
      // AS: To keep the selection working properly as we scroll,
      //  we fake a mouse event (remember, this method is called
      //  from a timer tick).

      // AS: For some reason, GCC won't let us pass this directly.
      wxMouseEvent evt(wxEVT_MOTION);
      const auto size = trackPanel.GetSize();
      mParent->Drag(
            TrackPanelMouseEvent{
                  evt, mParent->mRect, size,
                  TrackView::Get( *pTrack ).shared_from_this() },
            project
      );
      mParent->mAutoScrolling = false;
      TrackPanel::Get( *mConnectedProject ).Refresh(false);
   }
}
