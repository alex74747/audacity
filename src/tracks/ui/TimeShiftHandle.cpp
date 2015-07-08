/**********************************************************************

Audacity: A Digital Audio Editor

TimeShiftHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "TimeShiftHandle.h"

#include "TrackControls.h"
#include "../../AColor.h"
#include "../../HitTestResult.h"
#include "../../Project.h"
#include "../../RefreshCode.h"
#include "../../Snap.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../toolbars/ToolsToolBar.h"
#include "../../WaveTrack.h"
#include "../../../images/Cursors.h"

TimeShiftHandle::TimeShiftHandle()
   : mCapturedTrack(0)
   , mRect()
   , mCapturedClip(0)
   , mCapturedClipArray()
   , mTrackExclusions()
   , mCapturedClipIsSelection(false)
   , mHSlideAmount(0.0)
   , mDidSlideVertically(false)
   , mSlideUpDownOnly(false)
   , mSnapPreferRightEdge(false)
   , mMouseClickX(0)
   , mSnapManager()
   , mSnapLeft(0)
   , mSnapRight(0)
{
}

TimeShiftHandle &TimeShiftHandle::Instance()
{
   static TimeShiftHandle instance;
   return instance;
}

HitTestPreview TimeShiftHandle::HitPreview
(const AudacityProject *pProject, bool unsafe)
{
   static std::auto_ptr<wxCursor> disabledCursor(
      ::MakeCursor(wxCURSOR_NO_ENTRY, DisabledCursorXpm, 16, 16)
   );
   static std::auto_ptr<wxCursor> slideCursor(
      MakeCursor(wxCURSOR_SIZEWE, TimeCursorXpm, 16, 16)
   );
   const ToolsToolBar *const ttb = pProject->GetToolsToolBar();
   return HitTestPreview(
      ttb->GetMessageForTool(slideTool),
      (unsafe
       ? &*disabledCursor
       : &*slideCursor)
   );
}

HitTestResult TimeShiftHandle::HitAnywhere(const AudacityProject *pProject)
{
   // After all that, it still may be unsafe to drag.
   // Even if so, make an informative cursor change from default to "banned."
   const bool unsafe = pProject->IsAudioActive();
   return HitTestResult(
      HitPreview(pProject, unsafe),
      (unsafe
       ? NULL
       : &Instance())
   );
}

HitTestResult TimeShiftHandle::HitTest
   (const wxMouseEvent & event, const wxRect &rect, const AudacityProject *pProject)
{
   /// method that tells us if the mouse event landed on a
   /// time-slider that allows us to time shift the sequence.
   /// (Those are the two "grips" drawn at left and right edges for multi tool mode.)

   // Perhaps we should delegate this to TrackArtist as only TrackArtist
   // knows what the real sizes are??

   // The drag Handle width includes border, width and a little extra margin.
   const int adjustedDragHandleWidth = 14;
   // The hotspot for the cursor isn't at its centre.  Adjust for this.
   const int hotspotOffset = 5;

   // We are doing an approximate test here - is the mouse in the right or left border?
   if (!(event.m_x + hotspotOffset < rect.x + adjustedDragHandleWidth ||
       event.m_x + hotspotOffset >= rect.x + rect.width - adjustedDragHandleWidth))
      return HitTestResult();

   return HitAnywhere(pProject);
}

TimeShiftHandle::~TimeShiftHandle()
{
}

namespace
{
   // Adds a track's clips to mCapturedClipArray within a specified time
   void AddClipsToCaptured
      (TrackClipArray &capturedClipArray, Track *pTrack, double t0, double t1)
   {
      if (pTrack->GetKind() == Track::Wave)
      {
         WaveClipList::compatibility_iterator it =
            static_cast<WaveTrack*>(pTrack)->GetClipIterator();
         while (it)
         {
            WaveClip *const clip = it->GetData();

            if (!clip->AfterClip(t0) && !clip->BeforeClip(t1))
            {
               // Avoid getting clips that were already captured
               bool newClip = true;
               for (unsigned int ii = 0; newClip && ii < capturedClipArray.size(); ++ii)
                  newClip = (capturedClipArray[ii].clip != clip);
               if (newClip)
                  capturedClipArray.Add(TrackClip(pTrack, clip));
            }
            it = it->GetNext();
         }
      }
      else
      {
         // This handles label tracks rather heavy-handedly -- it would be nice to
         // treat individual labels like clips

         // Avoid adding a track twice
         bool newClip = true;
         for (unsigned int ii = 0; newClip && ii < capturedClipArray.size(); ++ii)
            newClip = (capturedClipArray[ii].track != pTrack);
         if (newClip) {
#ifdef USE_MIDI
            // do not add NoteTrack if the data is outside of time bounds
            if (pTrack->GetKind() == Track::Note) {
               if (pTrack->GetEndTime() < t0 || pTrack->GetStartTime() > t1)
                  return;
            }
#endif
            capturedClipArray.Add(TrackClip(pTrack, NULL));
         }
      }
   }

   // Helper for the above, adds a track's clips to mCapturedClipArray (eliminates
   // duplication of this logic)
   void AddClipsToCaptured
      (TrackClipArray &capturedClipArray,
       const ViewInfo &viewInfo, Track *pTrack, bool withinSelection)
   {
      if (withinSelection)
         AddClipsToCaptured(capturedClipArray, pTrack,
            viewInfo.selectedRegion.t0(), viewInfo.selectedRegion.t1());
      else
         AddClipsToCaptured(capturedClipArray, pTrack,
            pTrack->GetStartTime(), pTrack->GetEndTime());
   }
}

UIHandle::Result TimeShiftHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;
   const ViewInfo &viewInfo = pProject->GetViewInfo();

   Track *const pTrack = static_cast<Track*>(evt.pCell);

   using namespace RefreshCode;

   const bool unsafe = pProject->IsAudioActive();
   if (unsafe)
      return Cancelled;

   TrackList *const trackList = pProject->GetTracks();

   mHSlideAmount = 0.0;
   mDidSlideVertically = false;

   mTrackExclusions.Clear();

   ToolsToolBar *const ttb = pProject->GetToolsToolBar();
   const bool multiToolModeActive = (ttb && ttb->IsDown(multiTool));

   const double clickTime =
      viewInfo.PositionToTime(event.m_x, rect.x);
   mCapturedClipIsSelection =
      (pTrack->GetSelected() &&
       clickTime > viewInfo.selectedRegion.t0() &&
       clickTime < viewInfo.selectedRegion.t1());

   if ((pTrack->GetKind() == Track::Wave
#ifdef USE_MIDI
      || pTrack->GetKind() == Track::Note
#endif
      ) && !event.ShiftDown())
   {
#ifdef USE_MIDI
      if (pTrack->GetKind() == Track::Wave) {
#endif
         WaveTrack* wt = static_cast<WaveTrack*>(pTrack);
         mCapturedClip = wt->GetClipAtX(event.m_x);
         if (mCapturedClip == NULL)
            return Cancelled;
#ifdef USE_MIDI
      }
      else {
         mCapturedClip = NULL;
      }
#endif
      // The captured clip is the focus, but we need to create a list
      // of all clips that have to move, also...

      mCapturedClipArray.Clear();

      // First, if click was in selection, capture selected clips; otherwise
      // just the clicked-on clip
      if (mCapturedClipIsSelection) {
         TrackListIterator iter(trackList);
         for (Track *pTrack1 = iter.First(); pTrack1; pTrack1 = iter.Next()) {
            if (pTrack1->GetSelected()) {
               AddClipsToCaptured(mCapturedClipArray, viewInfo, pTrack1, true);
               if (pTrack1->GetKind() != Track::Wave)
                  mTrackExclusions.Add(pTrack1);
            }
         }
      }
      else {
         mCapturedClipArray.Add(TrackClip(pTrack, mCapturedClip));

         // Check for stereo partner
         Track *const partner = trackList->GetLink(pTrack);
         if (mCapturedClip && partner && partner->GetKind() == Track::Wave) {
            // WaveClip::GetClipAtX doesn't work unless the clip is on the screen and can return bad info otherwise
            // instead calculate the time manually
            double rate = static_cast<WaveTrack*>(partner)->GetRate();
            const double tt = viewInfo.PositionToTime(event.m_x, rect.x);
            sampleCount s0 = (sampleCount)(tt * rate + 0.5);

            if (s0 >= 0) {
               WaveClip *const clip =
                  static_cast<WaveTrack*>(partner)->GetClipAtSample(s0);
               if (clip) {
                  mCapturedClipArray.Add(TrackClip(partner, clip));
               }
            }
         }
      }

      // Now, if sync-lock is enabled, capture any clip that's linked to a
      // captured clip.
      if (pProject->IsSyncLocked()) {
         // AWD: mCapturedClipArray expands as the loop runs, so newly-added
         // clips are considered (the effect is like recursion and terminates
         // because AddClipsToCaptured doesn't add duplicate clips); to remove
         // this behavior just store the array size beforehand.
         for (unsigned int ii = 0; ii < mCapturedClipArray.size(); ++ii) {
            // Capture based on tracks that have clips -- that means we
            // don't capture based on links to label tracks for now (until
            // we can treat individual labels as clips)
            WaveClip *const clip = mCapturedClipArray[ii].clip;
            if (clip) {
               const double startTime = clip->GetStartTime();
               const double endTime = clip->GetEndTime();
               // Iterate over sync-lock group tracks.
               SyncLockedTracksIterator git(trackList);
               for (Track *pTrack1 = git.First(mCapturedClipArray[ii].track); pTrack1;
                  pTrack1 = git.Next()) {
                  AddClipsToCaptured(mCapturedClipArray, pTrack1, startTime, endTime);
                  if (pTrack1->GetKind() != Track::Wave)
                     mTrackExclusions.Add(pTrack1);
               }
            }
#ifdef USE_MIDI
            // Capture additional clips from NoteTracks
            Track *const nt = mCapturedClipArray[ii].track;
            if (nt->GetKind() == Track::Note) {
               const double startTime = nt->GetStartTime();
               const double endTime = nt->GetEndTime();
               // Iterate over sync-lock group tracks.
               SyncLockedTracksIterator git(trackList);
               for (Track *pTrack1 = git.First(nt); pTrack1; pTrack1 = git.Next()) {
                  AddClipsToCaptured(mCapturedClipArray, pTrack1, startTime, endTime);
                  if (pTrack1->GetKind() != Track::Wave)
                     mTrackExclusions.Add(pTrack1);
               }
            }
#endif
         }
      }
   }
   else {
      // Shift was down, or track was not Wave or Note
      mCapturedClip = NULL;
      mCapturedClipArray.Clear();
   }

   mSlideUpDownOnly = event.CmdDown() && !multiToolModeActive;
   mCapturedTrack = pTrack;
   mRect = rect;
   mMouseClickX = event.m_x;
   const double selStart = viewInfo.PositionToTime(event.m_x, mRect.x);
   mSnapManager.reset(new SnapManager(trackList,
      &viewInfo,
      &mCapturedClipArray,
      &mTrackExclusions,
      true // don't snap to time
   ));
   mSnapLeft = -1;
   mSnapRight = -1;
   mSnapPreferRightEdge =
      mCapturedClip &&
      (fabs(selStart - mCapturedClip->GetEndTime()) <
       fabs(selStart - mCapturedClip->GetStartTime()));

   return RefreshNone;
}

namespace
{
   bool MoveClipToTrack
      (TrackList *trackList, TrackClipArray &capturedClipArray,
       WaveClip *clip, WaveTrack* dst)
   {
      WaveTrack *src = NULL;
      WaveClip *clip2 = NULL;
      WaveTrack *src2 = NULL;
      WaveTrack *dst2 = NULL;

      for (size_t ii = 0; ii < capturedClipArray.size(); ++ii) {
         if (clip == capturedClipArray[ii].clip) {
            src = static_cast<WaveTrack*>(capturedClipArray[ii].track);
            break;
         }
      }

      if (!src)
         return false;

      // Make sure we have the first track of two stereo tracks
      // with both source and destination
      Track *link = NULL;
      if (!src->GetLinked() &&
          NULL != (link = trackList->GetLink(src))) {
         // set it to NULL in case there is no L channel clip
         clip = NULL;

         // find the first clip by getting the linked track from src
         // assumes that mCapturedArray[ii].clip and .track is not NULL.
         for (size_t ii = 0; ii < capturedClipArray.size(); ++ii) {
            if (link == capturedClipArray[ii].track) {
               clip = capturedClipArray[ii].clip;
               break;
            }
         }
         src = static_cast<WaveTrack*>(link);
      }
      if (!dst->GetLinked() && trackList->GetLink(dst))
         dst = static_cast<WaveTrack*>(trackList->GetLink(dst));

      // Get the second track of two stereo tracks
      src2 = static_cast<WaveTrack*>(trackList->GetLink(src));
      dst2 = static_cast<WaveTrack*>(trackList->GetLink(dst));

      if ((src2 && !dst2) || (dst2 && !src2))
         return false; // cannot move stereo- to mono track or other way around

      for (size_t ii = 0; ii < capturedClipArray.size(); ++ii) {
         if (capturedClipArray[ii].track == src2) {
            clip2 = capturedClipArray[ii].clip;
            break;
         }
      }

      // if only the right clip of a stereo pair is being dragged, use clip instead of clip2 to get mono behavior.
      if (!clip && clip2) {
         clip = clip2;
         src = src2;
         dst = dst2;
         clip2 = NULL;
         src2 = dst2 = NULL;
      }

      if (!dst->CanInsertClip(clip))
         return false;

      if (clip2) {
         // we should have a source and dest track
         if (!dst2 || !src2)
            return false;

         if (!dst2->CanInsertClip(clip2))
            return false;
      }

      src->MoveClipToTrack(clip, dst);
      if (src2)
         src2->MoveClipToTrack(clip2, dst2);

      // update the captured clip array.
      for (size_t ii = 0; ii < capturedClipArray.size(); ++ii) {
         if (clip && capturedClipArray[ii].clip == clip) {
            capturedClipArray[ii].track = dst;
         }
         else if (clip2 && capturedClipArray[ii].clip == clip2) {
            capturedClipArray[ii].track = dst2;
         }
      }

      return true;
   }
}

UIHandle::Result TimeShiftHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   ViewInfo &viewInfo = pProject->GetViewInfo();

   // We may switch pTrack to its stereo partner below
   Track *pTrack = dynamic_cast<Track*>(evt.pCell);

   // Uncommenting this permits drag to continue to work even over the controls area
   /*
   pTrack = static_cast<CommonTrackPanelCell*>(evt.pCell)->FindTrack();
   */

   if (!pTrack) {
      // Allow sliding if the pointer is not over any track, but only if x is
      // within the bounds of the tracks area.
      if (event.m_x >= mRect.GetX() &&
         event.m_x < mRect.GetX() + mRect.GetWidth())
          pTrack = mCapturedTrack;
   }

   if (!pTrack)
      return RefreshCode::RefreshNone;

   using namespace RefreshCode;
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe) {
      this->Cancel(pProject);
      return RefreshAll | Cancelled;
   }

   TrackList *const trackList = pProject->GetTracks();

   // GM: DoSlide now implementing snap-to
   // samples functionality based on sample rate.

   // Start by undoing the current slide amount; everything
   // happens relative to the original horizontal position of
   // each clip...
#ifdef USE_MIDI
   if (mCapturedClipArray.size())
#else
   if (mCapturedClip)
#endif
   {
      for (unsigned ii = 0; ii < mCapturedClipArray.size(); ++ii) {
         if (mCapturedClipArray[ii].clip)
            mCapturedClipArray[ii].clip->Offset(-mHSlideAmount);
         else
            mCapturedClipArray[ii].track->Offset(-mHSlideAmount);
      }
   }
   else {
      // Was a shift-click
      mCapturedTrack->Offset(-mHSlideAmount);
      Track *const link = trackList->GetLink(mCapturedTrack);
      if (link)
         link->Offset(-mHSlideAmount);
   }

   if (mCapturedClipIsSelection) {
      // Slide the selection, too
      viewInfo.selectedRegion.move(-mHSlideAmount);
   }
   mHSlideAmount = 0.0;

   double desiredSlideAmount;
   if (mSlideUpDownOnly) {
      desiredSlideAmount = 0.0;
   }
   else {
      desiredSlideAmount =
         viewInfo.PositionToTime(event.m_x) -
         viewInfo.PositionToTime(mMouseClickX);
      bool trySnap = false;
      double clipLeft = 0, clipRight = 0;
#ifdef USE_MIDI
      if (pTrack->GetKind() == Track::Wave) {
         WaveTrack *const mtw = static_cast<WaveTrack*>(pTrack);
         const double rate = mtw->GetRate();
         // set it to a sample point
         desiredSlideAmount = rint(desiredSlideAmount * rate) / rate;
      }

      // Adjust desiredSlideAmount using SnapManager
      if (mSnapManager.get() && mCapturedClipArray.size()) {
         trySnap = true;
         if (mCapturedClip) {
            clipLeft = mCapturedClip->GetStartTime() + desiredSlideAmount;
            clipRight = mCapturedClip->GetEndTime() + desiredSlideAmount;
         }
         else {
            clipLeft = mCapturedTrack->GetStartTime() + desiredSlideAmount;
            clipRight = mCapturedTrack->GetEndTime() + desiredSlideAmount;
         }
      }
#else
      {
         trySnap = true;
         const double rate = pTrack->GetRate();
         // set it to a sample point
         desiredSlideAmount = rint(desiredSlideAmount * rate) / rate;
         if (mSnapManager && mCapturedClip) {
            clipLeft = mCapturedClip->GetStartTime() + desiredSlideAmount;
            clipRight = mCapturedClip->GetEndTime() + desiredSlideAmount;
         }
      }
#endif
      if (trySnap)
      {
         double newClipLeft = clipLeft;
         double newClipRight = clipRight;

         bool dummy1, dummy2;
         mSnapManager->Snap(mCapturedTrack, clipLeft, false, &newClipLeft,
            &dummy1, &dummy2);
         mSnapManager->Snap(mCapturedTrack, clipRight, false, &newClipRight,
            &dummy1, &dummy2);

         // Only one of them is allowed to snap
         if (newClipLeft != clipLeft && newClipRight != clipRight) {
            // Un-snap the un-preferred edge
            if (mSnapPreferRightEdge)
               newClipLeft = clipLeft;
            else
               newClipRight = clipRight;
         }

         // Take whichever one snapped (if any) and compute the new desiredSlideAmount
         mSnapLeft = -1;
         mSnapRight = -1;
         if (newClipLeft != clipLeft) {
            const double difference = (newClipLeft - clipLeft);
            desiredSlideAmount += difference;
            mSnapLeft = viewInfo.TimeToPosition(newClipLeft, mRect.x);
         }
         else if (newClipRight != clipRight) {
            const double difference = (newClipRight - clipRight);
            desiredSlideAmount += difference;
            mSnapRight = viewInfo.TimeToPosition(newClipRight, mRect.x);
         }
      }
   }

   // Scroll during vertical drag.
   // EnsureVisible(pTrack); //vvv Gale says this has problems on Linux, per bug 393 thread. Revert for 2.0.2.

   //If the mouse is over a track that isn't the captured track,
   //drag the clip to the pTrack
   if (mCapturedClip &&
       pTrack != mCapturedTrack &&
       pTrack->GetKind() == Track::Wave
       /* && !mCapturedClipIsSelection*/)
   {
      // Make sure we always have the first linked track of a stereo track
      if (!pTrack->GetLinked() && trackList->GetLink(pTrack))
         pTrack = trackList->GetLink(pTrack);

      // Temporary apply the offset because we want to see if the
      // track fits with the desired offset
      for (unsigned ii = 0; ii < mCapturedClipArray.size(); ++ii)
         if (mCapturedClipArray[ii].clip)
            mCapturedClipArray[ii].clip->Offset(desiredSlideAmount);
      // See if it can be moved
      if (MoveClipToTrack(trackList, mCapturedClipArray,
            mCapturedClip, static_cast<WaveTrack*>(pTrack))) {
         mCapturedTrack = pTrack;
         mDidSlideVertically = true;

         if (mCapturedClipIsSelection) {
            // Slide the selection, too
            viewInfo.selectedRegion.move(desiredSlideAmount);
         }

         // Make the offset permanent; start from a "clean slate"
         mHSlideAmount = 0.0;
         desiredSlideAmount = 0.0;
         mMouseClickX = event.m_x;
      }
      else {
         // Undo the offset
         for (unsigned ii = 0; ii < mCapturedClipArray.size(); ++ii)
            if (mCapturedClipArray[ii].clip)
               mCapturedClipArray[ii].clip->Offset(-desiredSlideAmount);
      }
   }

   if (mSlideUpDownOnly)
      return RefreshAll;

   // Implement sliding within the track(s)
   // Determine desired amount to slide
   mHSlideAmount = desiredSlideAmount;

   if (mHSlideAmount == 0.0)
      return RefreshAll;

#ifdef USE_MIDI
   if (mCapturedClipArray.size())
#else
   if (mCapturedClip)
#endif
   {
      double allowed;
      double initialAllowed;
      const double safeBigDistance = 1000 + 2.0 * (trackList->GetEndTime() -
         trackList->GetStartTime());

      do { // loop to compute allowed, does not actually move anything yet
         initialAllowed = mHSlideAmount;

         for (unsigned ii = 0; ii < mCapturedClipArray.size(); ++ii) {
            WaveClip *const clip = mCapturedClipArray[ii].clip;
            if (clip) { // only audio clips are used to compute allowed
               WaveTrack *const track =
                  static_cast<WaveTrack *>(mCapturedClipArray[ii].track);
               // Move all other selected clips totally out of the way
               // temporarily because they're all moving together and
               // we want to find out if OTHER clips are in the way,
               // not one of the moving ones
               for (unsigned jj = 0; jj < mCapturedClipArray.size(); ++jj) {
                  WaveClip *const clip2 = mCapturedClipArray[jj].clip;
                  if (clip2 && clip2 != clip)
                     clip2->Offset(-safeBigDistance);
               }

               if (track->CanOffsetClip(clip, mHSlideAmount, &allowed)) {
                  if (mHSlideAmount != allowed) {
                     mHSlideAmount = allowed;
                     mSnapLeft = mSnapRight = -1; // see bug 1067
                  }
               }
               else {
                  mHSlideAmount = 0.0;
                  mSnapLeft = mSnapRight = -1; // see bug 1067
               }

               for (unsigned jj = 0; jj < mCapturedClipArray.size(); ++jj) {
                  WaveClip *const clip2 = mCapturedClipArray[jj].clip;
                  if (clip2 && clip2 != clip)
                     clip2->Offset(safeBigDistance);
               }
            }
         }
      } while (mHSlideAmount != initialAllowed);

      if (mHSlideAmount != 0.0) { // finally, here is where clips are moved
         unsigned int ii;
         for (ii = 0; ii < mCapturedClipArray.size(); ++ii) {
            Track *const track = mCapturedClipArray[ii].track;
            WaveClip *const clip = mCapturedClipArray[ii].clip;
            if (clip)
               clip->Offset(mHSlideAmount);
            else
               track->Offset(mHSlideAmount);
         }
      }
   }
   else {
      // For non wavetracks, specifically label tracks ...
      // Or for shift-(ctrl-)drag which moves all clips of a track together
      mCapturedTrack->Offset(mHSlideAmount);
      Track *const link = trackList->GetLink(mCapturedTrack);
      if (link)
         link->Offset(mHSlideAmount);
   }
   if (mCapturedClipIsSelection) {
      // Slide the selection, too
      viewInfo.selectedRegion.move(mHSlideAmount);
   }

   return RefreshAll;
}

HitTestPreview TimeShiftHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *pProject)
{
   return HitPreview(pProject, false);
}

UIHandle::Result TimeShiftHandle::Release
(const TrackPanelMouseEvent &, AudacityProject *pProject,
 wxWindow *)
{
   using namespace RefreshCode;
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe)
      return this->Cancel(pProject);

   Result result = RefreshNone;

   for (size_t ii = 0; ii < mCapturedClipArray.size(); ++ii)
   {
      TrackClip &trackClip = mCapturedClipArray[ii];
      WaveClip* pWaveClip = trackClip.clip;
      // Note that per TrackPanel::AddClipsToCaptured(Track *t, double t0, double t1),
      // in the non-WaveTrack case, the code adds a NULL clip to mCapturedClipArray,
      // so we have to check for that any time we're going to deref it.
      // Previous code did not check it here, and that caused bug 367 crash.
      if (pWaveClip &&
         trackClip.track != trackClip.origTrack)
      {
         // Now that user has dropped the clip into a different track,
         // make sure the sample rate matches the destination track (mCapturedTrack).
         pWaveClip->Resample(static_cast<WaveTrack*>(trackClip.track)->GetRate());
         pWaveClip->MarkChanged();
      }
   }

   mCapturedTrack = NULL;
   mSnapManager.reset(NULL);
   mCapturedClipArray.Clear();
   
   // Do not draw yellow lines
   if (mSnapLeft != -1 || mSnapRight != -1) {
      mSnapLeft = mSnapRight = -1;
      result |= RefreshAll;
   }

   if (!mDidSlideVertically && mHSlideAmount == 0)
      return result;

   wxString msg;
   bool consolidate;
   if (mDidSlideVertically) {
      msg.Printf(_("Moved clip to another track"));
      consolidate = false;
   }
   else {
      wxString direction = mHSlideAmount > 0 ?
         /* i18n-hint: a direction as in left or right.*/
         _("right") :
         /* i18n-hint: a direction as in left or right.*/
         _("left");
      /* i18n-hint: %s is a direction like left or right */
      msg.Printf(_("Time shifted tracks/clips %s %.02f seconds"),
         direction.c_str(), fabs(mHSlideAmount));
      consolidate = true;
   }
   pProject->PushState(msg, _("Time-Shift"),
      consolidate ? (PUSH_CONSOLIDATE) : (PUSH_AUTOSAVE));

   return result | FixScrollbars;
}

UIHandle::Result TimeShiftHandle::Cancel(AudacityProject *pProject)
{
   pProject->RollbackState();
   mCapturedTrack = NULL;
   mSnapManager.reset(NULL);
   mCapturedClipArray.Clear();
   return RefreshCode::RefreshAll;
}

void TimeShiftHandle::DrawExtras
(DrawingPass pass,
 wxDC * dc, const wxRegion &, const wxRect &)
{
   if (pass == Panel) {
      // Draw snap guidelines if we have any
      if (mSnapManager.get() && (mSnapLeft >= 0 || mSnapRight >= 0)) {
         AColor::SnapGuidePen(dc);
         if (mSnapLeft >= 0) {
            AColor::Line(*dc, (int)mSnapLeft, 0, mSnapLeft, 30000);
         }
         if (mSnapRight >= 0) {
            AColor::Line(*dc, (int)mSnapRight, 0, mSnapRight, 30000);
         }
      }
   }
}
