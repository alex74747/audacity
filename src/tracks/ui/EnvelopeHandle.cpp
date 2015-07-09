/**********************************************************************

Audacity: A Digital Audio Editor

EnvelopeHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "EnvelopeHandle.h"

#include <memory>

#include "../../Envelope.h"
#include "../../HitTestResult.h"
#include "../../prefs/WaveformSettings.h"
#include "../../Project.h"
#include "../../RefreshCode.h"
#include "../../toolbars/ToolsToolBar.h"
#include "../../TimeTrack.h"
#include "../../TrackArtist.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../ViewInfo.h"
#include "../../WaveTrack.h"
#include "../../../images/Cursors.h"

EnvelopeHandle::EnvelopeHandle()
   : mClickedEnvelope(NULL)
   , mPartner(NULL)
   , mRect()
   , mLog(false)
   , mLower(0.0f), mUpper(0.0f)
   , mdBRange(0.0)
{
}

EnvelopeHandle &EnvelopeHandle::Instance()
{
   static EnvelopeHandle instance;
   return instance;
}

HitTestPreview EnvelopeHandle::HitPreview(const AudacityProject *pProject, bool unsafe)
{
   static std::auto_ptr<wxCursor> disabledCursor(
      ::MakeCursor(wxCURSOR_NO_ENTRY, DisabledCursorXpm, 16, 16)
   );
   static std::auto_ptr<wxCursor> envelopeCursor(
      ::MakeCursor(wxCURSOR_ARROW, EnvCursorXpm, 16, 16)
   );
   const ToolsToolBar *const ttb = pProject->GetToolsToolBar();
   return HitTestPreview(
      ttb->GetMessageForTool(envelopeTool),
      (unsafe
       ? &*disabledCursor
       : &*envelopeCursor)
   );
}

HitTestResult EnvelopeHandle::HitAnywhere(const AudacityProject *pProject)
{
   const bool unsafe = pProject->IsAudioActive();
   return HitTestResult(
      HitPreview(pProject, unsafe),
      (unsafe
       ? NULL
       : &Instance())
   );
}

HitTestResult EnvelopeHandle::WaveTrackHitTest
(const wxMouseEvent &event, const wxRect &rect,
 const AudacityProject *pProject, Cell *pCell)
{
   const ViewInfo &viewInfo = pProject->GetViewInfo();
   Track *const pTrack = static_cast<Track*>(pCell);

   /// method that tells us if the mouse event landed on an
   /// envelope boundary.
   if (pTrack->GetKind() != Track::Wave)
      return HitTestResult();

   WaveTrack *const wavetrack = static_cast<WaveTrack*>(pTrack);
   Envelope *const envelope = wavetrack->GetEnvelopeAtX(event.GetX());

   if (!envelope)
      return HitTestResult();

   const int displayType = wavetrack->GetDisplay();
   // Not an envelope hit, unless we're using a type of wavetrack display
   // suitable for envelopes operations, ie one of the Wave displays.
   if (displayType != WaveTrack::Waveform)
      return HitTestResult();  // No envelope, not a hit, so return.

   // Get envelope point, range 0.0 to 1.0
   const bool dB = !wavetrack->GetWaveformSettings().isLinear();
   // Convert x to time.
   const double envValue =
      envelope->GetValue(viewInfo.PositionToTime(event.m_x, rect.x));

   float zoomMin, zoomMax;
   wavetrack->GetDisplayBounds(&zoomMin, &zoomMax);

   // Get y position of envelope point.
   const float dBRange = wavetrack->GetWaveformSettings().dBRange;
   int yValue = GetWaveYPos(envValue,
      zoomMin, zoomMax,
      rect.height, dB, true, dBRange, false) + rect.y;

   // Get y position of center line
   int ctr = GetWaveYPos(0.0,
      zoomMin, zoomMax,
      rect.height, dB, true, dBRange, false) + rect.y;

   // Get y distance of mouse from center line (in pixels).
   int yMouse = abs(ctr - event.m_y);
   // Get y distance of envelope from center line (in pixels)
   yValue = abs(ctr - yValue);

   // JKC: It happens that the envelope is actually drawn offset from its
   // 'true' position (it is 3 pixels wide).  yMisalign is really a fudge
   // factor to allow us to hit it exactly, but I wouldn't dream of
   // calling it yFudgeFactor :)
   const int yMisalign = 2;
   // Perhaps yTolerance should be put into preferences?
   const int yTolerance = 5; // how far from envelope we may be and count as a hit.
   int distance;

   // For amplification using the envelope we introduced the idea of contours.
   // The contours have the same shape as the envelope, which may be partially off-screen.
   // The contours are closer in to the center line.
   int ContourSpacing = (int)(rect.height / (2 * (zoomMax - zoomMin)));
   const int MaxContours = 2;

   // Adding ContourSpacing/2 selects a region either side of the contour.
   int yDisplace = yValue - yMisalign - yMouse + ContourSpacing / 2;
   if (yDisplace > (MaxContours * ContourSpacing))
      return HitTestResult();
   // Subtracting the ContourSpacing/2 we added earlier ensures distance is centred on the contour.
   distance = abs((yDisplace % ContourSpacing) - ContourSpacing / 2);
   if (distance >= yTolerance)
      return HitTestResult();

   return HitAnywhere(pProject);
}

EnvelopeHandle::~EnvelopeHandle()
{
}

UIHandle::Result EnvelopeHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   const ViewInfo &viewInfo = pProject->GetViewInfo();
   Track *const pTrack = static_cast<Track*>(evt.pCell);

   using namespace RefreshCode;
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe)
      return Cancelled;

   if (pTrack->GetKind() == Track::Wave) {
      WaveTrack *const wt = static_cast<WaveTrack*>(pTrack);
      if (wt->GetDisplay() != WaveTrack::Waveform)
         return Cancelled;

      mClickedEnvelope =
         wt->GetEnvelopeAtX(event.GetX());
      mPartner = static_cast<WaveTrack*>(wt->GetLink());
      mLog = !wt->GetWaveformSettings().isLinear();
      wt->GetDisplayBounds(&mLower, &mUpper);
      mdBRange = wt->GetWaveformSettings().dBRange;
   }
   else if (pTrack->GetKind() == Track::Time)
   {
      TimeTrack *const tt = static_cast<TimeTrack*>(pTrack);
      mClickedEnvelope = tt->GetEnvelope();
      mPartner = NULL;
      mLog = tt->GetDisplayLog();
      mLower = tt->GetRangeLower(), mUpper = tt->GetRangeUpper();
      if (mLog) {
         // MB: silly way to undo the work of GetWaveYPos while still getting a logarithmic scale
         mdBRange = viewInfo.dBr;
         mLower = LINEAR_TO_DB(std::max(1.0e-7, double(mLower))) / mdBRange + 1.0;
         mUpper = LINEAR_TO_DB(std::max(1.0e-7, double(mUpper))) / mdBRange + 1.0;
      }
   }
   else
      return Cancelled;

   mRect = evt.rect;

   const bool needUpdate = ForwardEventToEnvelopes(event, viewInfo);
   return needUpdate ? RefreshCell : RefreshNone;
}

UIHandle::Result EnvelopeHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   const wxMouseEvent &event = evt.event;
   const ViewInfo &viewInfo = pProject->GetViewInfo();
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe) {
      this->Cancel(pProject);
      return RefreshCell | Cancelled;
   }

   const bool needUpdate = ForwardEventToEnvelopes(event, viewInfo);
   return needUpdate ? RefreshCell : RefreshNone;
}

HitTestPreview EnvelopeHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *pProject)
{
   return HitPreview(pProject, false);
}

UIHandle::Result EnvelopeHandle::Release
(const TrackPanelMouseEvent &evt, AudacityProject *pProject,
 wxWindow *pParent)
{
   const wxMouseEvent &event = evt.event;
   const ViewInfo &viewInfo = pProject->GetViewInfo();
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe)
      return this->Cancel(pProject);

   const bool needUpdate = ForwardEventToEnvelopes(event, viewInfo);

   pProject->PushState(
      /* i18n-hint: (verb) Audacity has just adjusted the envelope .*/
      _("Adjusted envelope."),
      /* i18n-hint: The envelope is a curve that controls the audio loudness.*/
      _("Envelope")
   );

   mClickedEnvelope = NULL;
   mPartner = NULL;

   using namespace RefreshCode;
   return needUpdate ? RefreshCell : RefreshNone;
}

UIHandle::Result EnvelopeHandle::Cancel(AudacityProject *pProject)
{
   pProject->RollbackState();
   mClickedEnvelope = NULL;
   mPartner = NULL;
   return RefreshCode::RefreshCell;
}

bool EnvelopeHandle::ForwardEventToEnvelopes
   (const wxMouseEvent &event, const ViewInfo &viewInfo)
{
   /// The Envelope class actually handles things at the mouse
   /// event level, so we have to forward the events over.  Envelope
   /// will then tell us whether or not we need to redraw.

   // AS: I'm not sure why we can't let the Envelope take care of
   //  redrawing itself.  ?
   bool needUpdate =
      mClickedEnvelope->MouseEvent(
         event, mRect, viewInfo, mLog, mdBRange, mLower, mUpper);

   if (mPartner)
   {
      // wave track
      Envelope *e2 = mPartner->GetEnvelopeAtX(event.GetX());
      bool updateNeeded = e2 && e2->MouseEvent(
         event, mRect, viewInfo, mLog, mdBRange, mLower, mUpper);
      if (!updateNeeded) {   // no envelope found at this x point, or found but not updated
         if ((e2 = mPartner->GetActiveEnvelope()) != 0)  // search for any active DragPoint
            needUpdate |= e2->MouseEvent(
            event, mRect, viewInfo, mLog, mdBRange, mLower, mUpper);
      }
   }
   return needUpdate;
}
