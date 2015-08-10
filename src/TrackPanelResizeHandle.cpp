/**********************************************************************

Audacity: A Digital Audio Editor

TrackPanelResizeHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "TrackPanelResizeHandle.h"
#include "Experimental.h"

#include <memory>

#include <wx/cursor.h>
#include <wx/translation.h>

#include "HitTestResult.h"
#include "Project.h"
#include "RefreshCode.h"
#include "Track.h"
#include "TrackPanelMouseEvent.h"
#include "tracks/ui/TrackControls.h"
#include "tracks/ui/TrackVRulerControls.h"

TrackPanelResizeHandle::TrackPanelResizeHandle()
   : mMode(IsResizing)
   , mInitialMinimized(false)
   , mInitialTrackHeight(0)
   , mInitialActualHeight(0)
   , mInitialUpperTrackHeight(0)
   , mInitialUpperActualHeight(0)
   , mMouseClickY(0)
{
}

TrackPanelResizeHandle &TrackPanelResizeHandle::Instance()
{
   static TrackPanelResizeHandle instance;
   return instance;
}

HitTestPreview TrackPanelResizeHandle::HitPreview(bool bLinked)
{
   static std::auto_ptr<wxCursor> resizeCursor(
      new wxCursor(wxCURSOR_SIZENS)
   );

   /// When in the resize area we can adjust size or relative size.
   // Check to see whether it is the first channel of a stereo track
   if (bLinked) {
      // If we are in the label we got here 'by mistake' and we're
      // not actually in the resize area at all.  (The resize area
      // is shorter when it is between stereo tracks).

      return HitTestPreview(
         _("Click and drag to adjust relative size of stereo tracks."),
         &*resizeCursor
      );
   }
   else {
      return HitTestPreview(
         _("Click and drag to resize the track."),
         &*resizeCursor
      );
   }
}

TrackPanelResizeHandle::~TrackPanelResizeHandle()
{
}

UIHandle::Result TrackPanelResizeHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   Track *track = dynamic_cast<Track*>(evt.pCell);
   if (!track) {
      // Clicked under a label or ruler;
      // if stereo, replace left channel with the right:
      CommonTrackPanelCell *const pCell =
         static_cast<CommonTrackPanelCell*>(evt.pCell);
      track = pCell->FindTrack();
      if (track && track->GetLinked())
         track = track->GetLink();
   }
   if (!track)
      return RefreshCode::Cancelled;

   mpTrack = track;

   ///  ButtonDown means they just clicked and haven't released yet.
   ///  We use this opportunity to save which track they clicked on,
   ///  and the initial height of the track, so as they drag we can
   ///  update the track size.
   wxRect rTrack;
   wxRect rLabel;

   mMouseClickY = event.m_y;

   TrackList *const tracks = pProject->GetTracks();

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   // To do: escape key
   if (MONO_WAVE_PAN(track)){
      //STM:  Determine whether we should rescale one or two tracks
      if (track->GetVirtualStereo()) {
         // mpTrack is the lower track
         mInitialTrackHeight = track->GetHeight(true);
         mInitialUpperTrackHeight = track->GetHeight();
         SetCapturedTrack(track, IsResizingBelowLinkedTracks);
      }
      else {
         // mpTrack is the upper track
         mInitialTrackHeight = track->GetHeight(true);
         mInitialUpperTrackHeight = track->GetHeight();
         SetCapturedTrack(track, IsResizingBetweenLinkedTracks);
      }
   }
   else{
      Track *prev = tracks->GetPrev(track);
      Track *next = tracks->GetNext(track);

      //STM:  Determine whether we should rescale one or two tracks
      if (prev && prev->GetLink() == track) {
         // mpTrack is the lower track
         mInitialTrackHeight = track->GetHeight();
         mInitialMinimized = track->GetMinimized();
         mInitialUpperTrackHeight = prev->GetHeight();
         mMode = IsResizingBelowLinkedTracks;
      }
      else if (next && track->GetLink() == next) {
         // mpTrack is the upper track
         mInitialTrackHeight = next->GetHeight();
         mInitialMinimized = next->GetMinimized();
         mInitialUpperTrackHeight = track->GetHeight();
         mMode = IsResizingBetweenLinkedTracks;
      }
      else {
         // DM: Save the initial mouse location and the initial height
         mInitialTrackHeight = track->GetHeight();
         mInitialMinimized = track->GetMinimized();
         mMode = IsResizing;
      }
   }
#else // EXPERIMENTAL_OUTPUT_DISPLAY
   Track *prev = tracks->GetPrev(track);
   Track *next = tracks->GetNext(track);

   //STM:  Determine whether we should rescale one or two tracks
   if (prev && prev->GetLink() == track) {
      // mpTrack is the lower track
      mInitialTrackHeight = track->GetHeight();
      mInitialActualHeight = track->GetActualHeight();
      mInitialMinimized = track->GetMinimized();
      mInitialUpperTrackHeight = prev->GetHeight();
      mInitialUpperActualHeight = prev->GetActualHeight();
      mMode = IsResizingBelowLinkedTracks;
   }
   else if (next && track->GetLink() == next) {
      // mpTrack is the upper track
      mInitialTrackHeight = next->GetHeight();
      mInitialActualHeight = next->GetActualHeight();
      mInitialMinimized = next->GetMinimized();
      mInitialUpperTrackHeight = track->GetHeight();
      mInitialUpperActualHeight = track->GetActualHeight();
      mMode = IsResizingBetweenLinkedTracks;
   }
   else {
      // DM: Save the initial mouse location and the initial height
      mInitialTrackHeight = track->GetHeight();
      mInitialActualHeight = track->GetActualHeight();
      mInitialMinimized = track->GetMinimized();
      mMode = IsResizing;
   }
#endif // EXPERIMENTAL_OUTPUT_DISPLAY

   return RefreshCode::RefreshNone;
}

UIHandle::Result TrackPanelResizeHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   TrackList *const tracks = pProject->GetTracks();

   int delta = (event.m_y - mMouseClickY);

   // On first drag, jump out of minimized mode.  Initial height
   // will be height of minimized track.
   //
   // This used to be in HandleResizeClick(), but simply clicking
   // on a resize border would switch the minimized state.
   if (mpTrack->GetMinimized()) {
      Track *link = mpTrack->GetLink();

      mpTrack->SetHeight(mpTrack->GetHeight());
      mpTrack->SetMinimized(false);

      if (link) {
         link->SetHeight(link->GetHeight());
         link->SetMinimized(false);
         // Initial values must be reset since they weren't based on the
         // minimized heights.
         mInitialUpperTrackHeight = link->GetHeight();
         mInitialTrackHeight = mpTrack->GetHeight();
      }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      else if (MONO_WAVE_PAN(mpTrack)){
         mpTrack->SetMinimized(false);
         mInitialUpperTrackHeight = mpTrack->GetHeight();
         mInitialTrackHeight = mpTrack->GetHeight(true);
      }
#endif
   }

   //STM: We may be dragging one or two (stereo) tracks.
   // If two, resize proportionally if we are dragging the lower track, and
   // adjust compensatively if we are dragging the upper track.
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   switch (mMode)
   {
   case IsResizingBelowLinkedTracks:
   {
      if (MONO_WAVE_PAN(mpTrack)){
         double proportion = static_cast < double >(mInitialTrackHeight)
            / (mInitialTrackHeight + mInitialUpperTrackHeight);

         int newTrackHeight = static_cast < int >
            (mInitialTrackHeight + delta * proportion);

         int newUpperTrackHeight = static_cast < int >
            (mInitialUpperTrackHeight + delta * (1.0 - proportion));

         //make sure neither track is smaller than its minimum height
         if (newTrackHeight < mpTrack->GetMinimizedHeight())
            newTrackHeight = mpTrack->GetMinimizedHeight();
         if (newUpperTrackHeight < mpTrack->GetMinimizedHeight())
            newUpperTrackHeight = mpTrack->GetMinimizedHeight();

         mpTrack->SetHeight(newTrackHeight, true);
         mpTrack->SetHeight(newUpperTrackHeight);
      }
      else{
         Track *prev = tracks->GetPrev(mpTrack);

         double proportion = static_cast < double >(mInitialTrackHeight)
            / (mInitialTrackHeight + mInitialUpperTrackHeight);

         int newTrackHeight = static_cast < int >
            (mInitialTrackHeight + delta * proportion);

         int newUpperTrackHeight = static_cast < int >
            (mInitialUpperTrackHeight + delta * (1.0 - proportion));

         //make sure neither track is smaller than its minimum height
         if (newTrackHeight < mpTrack->GetMinimizedHeight())
            newTrackHeight = mpTrack->GetMinimizedHeight();
         if (newUpperTrackHeight < prev->GetMinimizedHeight())
            newUpperTrackHeight = prev->GetMinimizedHeight();

         mpTrack->SetHeight(newTrackHeight);
         prev->SetHeight(newUpperTrackHeight);
      }
      break;
   }
   case IsResizingBetweenLinkedTracks:
   {
      if (MONO_WAVE_PAN(mpTrack)){
         int newUpperTrackHeight = mInitialUpperTrackHeight + delta;
         int newTrackHeight = mInitialTrackHeight - delta;

         // make sure neither track is smaller than its minimum height
         if (newTrackHeight < mpTrack->GetMinimizedHeight()) {
            newTrackHeight = mpTrack->GetMinimizedHeight();
            newUpperTrackHeight =
               mInitialUpperTrackHeight + mInitialTrackHeight - mpTrack->GetMinimizedHeight();
         }
         if (newUpperTrackHeight < mpTrack->GetMinimizedHeight()) {
            newUpperTrackHeight = mpTrack->GetMinimizedHeight();
            newTrackHeight =
               mInitialUpperTrackHeight + mInitialTrackHeight - mpTrack->GetMinimizedHeight();
         }
         float temp = 1.0f;
         if (newUpperTrackHeight != 0.0f)
            temp = (float)newUpperTrackHeight / (float)(newUpperTrackHeight + newTrackHeight);

         mpTrack->SetVirtualTrackPercentage(temp);
         mpTrack->SetHeight(newUpperTrackHeight);
         mpTrack->SetHeight(newTrackHeight, true);
      }
      else{
         Track *next = tracks->GetNext(mpTrack);
         int newUpperTrackHeight = mInitialUpperTrackHeight + delta;
         int newTrackHeight = mInitialTrackHeight - delta;

         // make sure neither track is smaller than its minimum height
         if (newTrackHeight < next->GetMinimizedHeight()) {
            newTrackHeight = next->GetMinimizedHeight();
            newUpperTrackHeight =
               mInitialUpperTrackHeight + mInitialTrackHeight - next->GetMinimizedHeight();
         }
         if (newUpperTrackHeight < mpTrack->GetMinimizedHeight()) {
            newUpperTrackHeight = mpTrack->GetMinimizedHeight();
            newTrackHeight =
               mInitialUpperTrackHeight + mInitialTrackHeight - mpTrack->GetMinimizedHeight();
         }

         mpTrack->SetHeight(newUpperTrackHeight);
         next->SetHeight(newTrackHeight);
         break;
      }
      break;
   }
   case IsResizing:
   {
      int newTrackHeight = mInitialTrackHeight + delta;
      if (newTrackHeight < mpTrack->GetMinimizedHeight())
         newTrackHeight = mpTrack->GetMinimizedHeight();
      mpTrack->SetHeight(newTrackHeight);
      break;
   }
   default:
      // don't refresh in this case.
      return RefreshCode::RefreshNone;
   }
#else // EXPERIMENTAL_OUTPUT_DISPLAY

   switch (mMode)
   {
   case IsResizingBelowLinkedTracks:
   {
      Track *prev = tracks->GetPrev(mpTrack);

      double proportion = static_cast <double>(mInitialTrackHeight)
         / (mInitialTrackHeight + mInitialUpperTrackHeight);

      int newTrackHeight = static_cast <int>
         (mInitialTrackHeight + delta * proportion);

      int newUpperTrackHeight = static_cast <int>
         (mInitialUpperTrackHeight + delta * (1.0 - proportion));

      //make sure neither track is smaller than its minimum height
      if (newTrackHeight < mpTrack->GetMinimizedHeight())
         newTrackHeight = mpTrack->GetMinimizedHeight();
      if (newUpperTrackHeight < prev->GetMinimizedHeight())
         newUpperTrackHeight = prev->GetMinimizedHeight();

      mpTrack->SetHeight(newTrackHeight);
      prev->SetHeight(newUpperTrackHeight);
      break;
   }
   case IsResizingBetweenLinkedTracks:
   {
      Track *next = tracks->GetNext(mpTrack);
      int newUpperTrackHeight = mInitialUpperTrackHeight + delta;
      int newTrackHeight = mInitialTrackHeight - delta;

      // make sure neither track is smaller than its minimum height
      if (newTrackHeight < next->GetMinimizedHeight()) {
         newTrackHeight = next->GetMinimizedHeight();
         newUpperTrackHeight =
            mInitialUpperTrackHeight + mInitialTrackHeight - next->GetMinimizedHeight();
      }
      if (newUpperTrackHeight < mpTrack->GetMinimizedHeight()) {
         newUpperTrackHeight = mpTrack->GetMinimizedHeight();
         newTrackHeight =
            mInitialUpperTrackHeight + mInitialTrackHeight - mpTrack->GetMinimizedHeight();
      }

      mpTrack->SetHeight(newUpperTrackHeight);
      next->SetHeight(newTrackHeight);
      break;
   }
   case IsResizing:
   {
      int newTrackHeight = mInitialTrackHeight + delta;
      if (newTrackHeight < mpTrack->GetMinimizedHeight())
         newTrackHeight = mpTrack->GetMinimizedHeight();
      mpTrack->SetHeight(newTrackHeight);
      break;
   }
   default:
      // don't refresh in this case.
      return RefreshCode::RefreshNone;
   }
#endif // EXPERIMENTAL_OUTPUT_DISPLAY

   return RefreshCode::RefreshAll;
}

HitTestPreview TrackPanelResizeHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *)
{
   return HitPreview(mMode == IsResizingBetweenLinkedTracks);
}

UIHandle::Result TrackPanelResizeHandle::Release
(const TrackPanelMouseEvent &, AudacityProject *pProject,
 wxWindow *)
{
   ///  This happens when the button is released from a drag.
   ///  Since we actually took care of resizing the track when
   ///  we got drag events, all we have to do here is clean up.
   ///  We also modify the undo state (the action doesn't become
   ///  undo-able, but it gets merged with the previous undo-able
   ///  event).
   pProject->ModifyState(false);
   return RefreshCode::FixScrollbars;
}

UIHandle::Result TrackPanelResizeHandle::Cancel(AudacityProject *pProject)
{
   TrackList *const tracks = pProject->GetTracks();

   switch (mMode) {
   case IsResizing:
   {
      mpTrack->SetHeight(mInitialActualHeight);
      mpTrack->SetMinimized(mInitialMinimized);
   }
   break;
   case IsResizingBetweenLinkedTracks:
   {
      Track *const next = tracks->GetNext(mpTrack);
      mpTrack->SetHeight(mInitialUpperActualHeight);
      mpTrack->SetMinimized(mInitialMinimized);
      next->SetHeight(mInitialActualHeight);
      next->SetMinimized(mInitialMinimized);
   }
   break;
   case IsResizingBelowLinkedTracks:
   {
      Track *const prev = tracks->GetPrev(mpTrack);
      mpTrack->SetHeight(mInitialActualHeight);
      mpTrack->SetMinimized(mInitialMinimized);
      prev->SetHeight(mInitialUpperActualHeight);
      prev->SetMinimized(mInitialMinimized);
   }
   break;
   }

   return RefreshCode::RefreshAll;
}
