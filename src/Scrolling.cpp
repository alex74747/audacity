#include "Audacity.h"
#include "AudioIO.h"
#include "Experimental.h"
#include "Menus.h"
#include "Project.h"
#include "Scrolling.h"
#include "TrackPanel.h"
#include "WaveTrack.h"
#include "prefs/TracksPrefs.h"
#include "tracks/ui/Scrubbing.h"

#if defined(__WXMAC__)
// const int sbarSpaceWidth = 15;
// const int sbarControlWidth = 16;
// const int sbarExtraLen = 1;
const int sbarHjump = 30;       //STM: This is how far the thumb jumps when the l/r buttons are pressed, or auto-scrolling occurs -- in pixels
#elif defined(__WXMSW__)
const int sbarSpaceWidth = 16;
const int sbarControlWidth = 16;
const int sbarExtraLen = 0;
const int sbarHjump = 30;       //STM: This is how far the thumb jumps when the l/r buttons are pressed, or auto-scrolling occurs -- in pixels
#else // wxGTK, wxMOTIF, wxX11
const int sbarSpaceWidth = 15;
const int sbarControlWidth = 15;
const int sbarExtraLen = 0;
const int sbarHjump = 30;       //STM: This is how far the thumb jumps when the l/r buttons are pressed, or auto-scrolling occurs -- in pixels
#include "Theme.h"
#include "AllThemeResources.h"
#endif

void ScrollBar::SetScrollbar(int position, int thumbSize,
                             int range, int pageSize,
                             bool refresh)
{
   // Mitigate flashing of scrollbars by refreshing only when something really changes.

   // PRL:  This may have been made unnecessary by other fixes for flashing, see
   // commit ac05b190bee7dd0000bce56edb0e5e26185c972f

   auto changed =
      position != GetThumbPosition() ||
      thumbSize != GetThumbSize() ||
      range != GetRange() ||
      pageSize != GetPageSize();
   if (!changed)
      return;

   wxScrollBar::SetScrollbar(position, thumbSize, range, pageSize, refresh);
}

BEGIN_EVENT_TABLE(ScrollBar, wxScrollBar)
   EVT_SET_FOCUS(ScrollBar::OnSetFocus)
END_EVENT_TABLE()

void AudacityProject::FinishAutoScroll()
{
   // Set a flag so we don't have to generate two update events
   mAutoScrolling = true;

   // Call our Scroll method which updates our ViewInfo variables
   // to reflect the positions of the scrollbars
   DoScroll();

   mAutoScrolling = false;
}


///
/// This method handles general left-scrolling, either for drag-scrolling
/// or when the scrollbar is clicked to the left of the thumb
///
void AudacityProject::OnScrollLeft()
{
   wxInt64 pos = mHsbar->GetThumbPosition();
   // move at least one scroll increment
   pos -= wxMax((wxInt64)(sbarHjump * mViewInfo.sbarScale), 1);
   pos = wxMax(pos, 0);
   mViewInfo.sbarH -= sbarHjump;
   mViewInfo.sbarH = std::max(mViewInfo.sbarH,
      -(wxInt64) PixelWidthBeforeTime(0.0));


   if (pos != mHsbar->GetThumbPosition()) {
      mHsbar->SetThumbPosition((int)pos);
      FinishAutoScroll();
   }
}
///
/// This method handles general right-scrolling, either for drag-scrolling
/// or when the scrollbar is clicked to the right of the thumb
///

void AudacityProject::OnScrollRight()
{
   wxInt64 pos = mHsbar->GetThumbPosition();
   // move at least one scroll increment
   // use wxInt64 for calculation to prevent temporary overflow
   pos += wxMax((wxInt64)(sbarHjump * mViewInfo.sbarScale), 1);
   wxInt64 max = mHsbar->GetRange() - mHsbar->GetThumbSize();
   pos = wxMin(pos, max);
   mViewInfo.sbarH += sbarHjump;
   mViewInfo.sbarH = std::min(mViewInfo.sbarH,
      mViewInfo.sbarTotal
         - (wxInt64) PixelWidthBeforeTime(0.0) - mViewInfo.sbarScreen);

   if (pos != mHsbar->GetThumbPosition()) {
      mHsbar->SetThumbPosition((int)pos);
      FinishAutoScroll();
   }
}

///
///  This handles the event when the left direction button on the scrollbar is depresssed
///
void AudacityProject::OnScrollLeftButton(wxScrollEvent & /*event*/)
{
   wxInt64 pos = mHsbar->GetThumbPosition();
   // move at least one scroll increment
   pos -= wxMax((wxInt64)(sbarHjump * mViewInfo.sbarScale), 1);
   pos = wxMax(pos, 0);
   mViewInfo.sbarH -= sbarHjump;
   mViewInfo.sbarH = std::max(mViewInfo.sbarH,
      - (wxInt64) PixelWidthBeforeTime(0.0));

   if (pos != mHsbar->GetThumbPosition()) {
      mHsbar->SetThumbPosition((int)pos);
      DoScroll();
   }
}

///
///  This handles  the event when the right direction button on the scrollbar is depresssed
///
void AudacityProject::OnScrollRightButton(wxScrollEvent & /*event*/)
{
   wxInt64 pos = mHsbar->GetThumbPosition();
   // move at least one scroll increment
   // use wxInt64 for calculation to prevent temporary overflow
   pos += wxMax((wxInt64)(sbarHjump * mViewInfo.sbarScale), 1);
   wxInt64 max = mHsbar->GetRange() - mHsbar->GetThumbSize();
   pos = wxMin(pos, max);
   mViewInfo.sbarH += sbarHjump;
   mViewInfo.sbarH = std::min(mViewInfo.sbarH,
      mViewInfo.sbarTotal
         - (wxInt64) PixelWidthBeforeTime(0.0) - mViewInfo.sbarScreen);

   if (pos != mHsbar->GetThumbPosition()) {
      mHsbar->SetThumbPosition((int)pos);
      DoScroll();
   }
}


bool AudacityProject::MayScrollBeyondZero() const
{
   if (mViewInfo.bScrollBeyondZero)
      return true;

   if (GetScrubber().HasMark() ||
       IsAudioActive()) {
      if (mPlaybackScroller) {
         auto mode = mPlaybackScroller->GetMode();
         if (mode == PlaybackScroller::Mode::Pinned ||
             mode == PlaybackScroller::Mode::Right)
            return true;
      }
   }

   return false;
}

double AudacityProject::ScrollingLowerBoundTime() const
{
   if (!MayScrollBeyondZero())
      return 0;
   const double screen = mTrackPanel->GetScreenEndTime() - mViewInfo.h;
   return std::min(mTracks->GetStartTime(), -screen);
}

// PRL: Bug1197: we seem to need to compute all in double, to avoid differing results on Mac
// That's why ViewInfo::TimeRangeToPixelWidth was defined, with some regret.
double AudacityProject::PixelWidthBeforeTime(double scrollto) const
{
   const double lowerBound = ScrollingLowerBoundTime();
   return
      // Ignoring fisheye is correct here
      mViewInfo.TimeRangeToPixelWidth(scrollto - lowerBound);
}

void AudacityProject::SetHorizontalThumb(double scrollto)
{
   const auto unscaled = PixelWidthBeforeTime(scrollto);
   const int max = mHsbar->GetRange() - mHsbar->GetThumbSize();
   const int pos =
      std::min(max,
         std::max(0,
            (int)(floor(0.5 + unscaled * mViewInfo.sbarScale))));
   mHsbar->SetThumbPosition(pos);
   mViewInfo.sbarH = floor(0.5 + unscaled - PixelWidthBeforeTime(0.0));
   mViewInfo.sbarH = std::max(mViewInfo.sbarH,
      - (wxInt64) PixelWidthBeforeTime(0.0));
   mViewInfo.sbarH = std::min(mViewInfo.sbarH,
      mViewInfo.sbarTotal
         - (wxInt64) PixelWidthBeforeTime(0.0) - mViewInfo.sbarScreen);
}

//
// This method, like the other methods prefaced with TP, handles TrackPanel
// 'callback'.
//
void AudacityProject::TP_ScrollWindow(double scrollto)
{
   SetHorizontalThumb(scrollto);

   // Call our Scroll method which updates our ViewInfo variables
   // to reflect the positions of the scrollbars
   DoScroll();
}

//
// Scroll vertically. This is called for example by the mouse wheel
// handler in Track Panel. A positive argument makes the window
// scroll down, while a negative argument scrolls up.
//
bool AudacityProject::TP_ScrollUpDown(int delta)
{
   int oldPos = mVsbar->GetThumbPosition();
   int pos = oldPos + delta;
   int max = mVsbar->GetRange() - mVsbar->GetThumbSize();

   // Can be negative in case of only one track
   if (max < 0)
      max = 0;

   if (pos > max)
      pos = max;
   else if (pos < 0)
      pos = 0;

   if (pos != oldPos)
   {
      mVsbar->SetThumbPosition(pos);

      DoScroll();
      return true;
   }
   else
      return false;
}

void AudacityProject::FixScrollbars()
{
   if (!GetTracks())
      return;

   bool refresh = false;
   bool rescroll = false;

   int totalHeight = (mTracks->GetHeight() + 32);

   int panelWidth, panelHeight;
   mTrackPanel->GetTracksUsableArea(&panelWidth, &panelHeight);

   // (From Debian...at least I think this is the change cooresponding
   // to this comment)
   //
   // (2.) GTK critical warning "IA__gtk_range_set_range: assertion
   // 'min < max' failed" because of negative numbers as result of window
   // size checking. Added a sanity check that straightens up the numbers
   // in edge cases.
   if (panelWidth < 0) {
      panelWidth = 0;
   }
   if (panelHeight < 0) {
      panelHeight = 0;
   }

   auto LastTime = std::numeric_limits<double>::lowest();
   auto &tracks = *GetTracks();
   for (const Track *track : tracks) {
      // Iterate over pending changed tracks if present.
      track = track->SubstitutePendingChangedTrack().get();
      LastTime = std::max( LastTime, track->GetEndTime() );
   }
   LastTime =
      std::max(LastTime, mViewInfo.selectedRegion.t1());

   const double screen = GetScreenEndTime() - mViewInfo.h;
   const double halfScreen = screen / 2.0;

   // If we can scroll beyond zero,
   // Add 1/2 of a screen of blank space to the end
   // and another 1/2 screen before the beginning
   // so that any point within the union of the selection and the track duration
   // may be scrolled to the midline.
   // May add even more to the end, so that you can always scroll the starting time to zero.
   const double lowerBound = ScrollingLowerBoundTime();
   const double additional = MayScrollBeyondZero()
      ? -lowerBound + std::max(halfScreen, screen - LastTime)
      : screen / 4.0;

   mViewInfo.total = LastTime + additional;

   // Don't remove time from total that's still on the screen
   mViewInfo.total = std::max(mViewInfo.total, mViewInfo.h + screen);

   if (mViewInfo.h < lowerBound) {
      mViewInfo.h = lowerBound;
      rescroll = true;
   }

   mViewInfo.sbarTotal = (wxInt64) (mViewInfo.GetTotalWidth());
   mViewInfo.sbarScreen = (wxInt64)(panelWidth);
   mViewInfo.sbarH = (wxInt64) (mViewInfo.GetBeforeScreenWidth());

   // PRL:  Can someone else find a more elegant solution to bug 812, than
   // introducing this boolean member variable?
   // Setting mVSbar earlier, int HandlXMLTag, didn't succeed in restoring
   // the vertical scrollbar to its saved position.  So defer that till now.
   // mbInitializingScrollbar should be true only at the start of the life
   // of an AudacityProject reopened from disk.
   if (!mbInitializingScrollbar) {
      mViewInfo.vpos = mVsbar->GetThumbPosition() * mViewInfo.scrollStep;
   }
   mbInitializingScrollbar = false;

   if (mViewInfo.vpos >= totalHeight)
      mViewInfo.vpos = totalHeight - 1;
   if (mViewInfo.vpos < 0)
      mViewInfo.vpos = 0;

   bool oldhstate;
   bool oldvstate;
   bool newhstate = (GetScreenEndTime() - mViewInfo.h) < mViewInfo.total;
   bool newvstate = panelHeight < totalHeight;

#ifdef __WXGTK__
   oldhstate = mHsbar->IsShown();
   oldvstate = mVsbar->IsShown();
   mHsbar->Show(newhstate);
   mVsbar->Show(panelHeight < totalHeight);
#else
   oldhstate = mHsbar->IsEnabled();
   oldvstate = mVsbar->IsEnabled();
   mHsbar->Enable(newhstate);
   mVsbar->Enable(panelHeight < totalHeight);
#endif

   if (panelHeight >= totalHeight && mViewInfo.vpos != 0) {
      mViewInfo.vpos = 0;

      refresh = true;
      rescroll = false;
   }
   if (!newhstate && mViewInfo.sbarH != 0) {
      mViewInfo.sbarH = 0;

      refresh = true;
      rescroll = false;
   }

   // wxScrollbar only supports int values but we need a greater range, so
   // we scale the scrollbar coordinates on demand. We only do this if we
   // would exceed the int range, so we can always use the maximum resolution
   // available.

   // Don't use the full 2^31 max int range but a bit less, so rounding
   // errors in calculations do not overflow max int
   wxInt64 maxScrollbarRange = (wxInt64)(2147483647 * 0.999);
   if (mViewInfo.sbarTotal > maxScrollbarRange)
      mViewInfo.sbarScale = ((double)maxScrollbarRange) / mViewInfo.sbarTotal;
   else
      mViewInfo.sbarScale = 1.0; // use maximum resolution

   {
      int scaledSbarH = (int)(mViewInfo.sbarH * mViewInfo.sbarScale);
      int scaledSbarScreen = (int)(mViewInfo.sbarScreen * mViewInfo.sbarScale);
      int scaledSbarTotal = (int)(mViewInfo.sbarTotal * mViewInfo.sbarScale);
      const int offset =
         (int)(floor(0.5 + mViewInfo.sbarScale * PixelWidthBeforeTime(0.0)));

      mHsbar->SetScrollbar(scaledSbarH + offset, scaledSbarScreen, scaledSbarTotal,
         scaledSbarScreen, TRUE);
   }

   // Vertical scrollbar
   mVsbar->SetScrollbar(mViewInfo.vpos / mViewInfo.scrollStep,
                        panelHeight / mViewInfo.scrollStep,
                        totalHeight / mViewInfo.scrollStep,
                        panelHeight / mViewInfo.scrollStep, TRUE);

   if (refresh || (rescroll &&
       (GetScreenEndTime() - mViewInfo.h) < mViewInfo.total)) {
      mTrackPanel->Refresh(false);
   }

   GetMenuManager(*this).UpdateMenus(*this);

   if (oldhstate != newhstate || oldvstate != newvstate) {
      UpdateLayout();
   }

   CallAfter(
      [this]{ if (GetTrackPanel())
         GetTrackPanel()->HandleCursorForPresentMouseState(); } );
}

void AudacityProject::HandleResize()
{
   if (!mTrackPanel)
      return;

   FixScrollbars();

   UpdateLayout();
}

void AudacityProject::OnSize(wxSizeEvent & event)
{
   // (From Debian)
   //
   // (3.) GTK critical warning "IA__gdk_window_get_origin: assertion
   // 'GDK_IS_WINDOW (window)' failed": Received events of type wxSizeEvent
   // on the main project window cause calls to "ClientToScreen" - which is
   // not available until the window is first shown. So the class has to
   // keep track of wxShowEvent events and inhibit those actions until the
   // window is first shown.
   if (mShownOnce) {
      HandleResize();
      if (!this->IsMaximized() && !this->IsIconized())
         SetNormalizedWindowState(this->GetRect());
   }
   event.Skip();
}

void AudacityProject::OnScroll(wxScrollEvent & WXUNUSED(event))
{
   const wxInt64 offset = PixelWidthBeforeTime(0.0);
   mViewInfo.sbarH =
      (wxInt64)(mHsbar->GetThumbPosition() / mViewInfo.sbarScale) - offset;
   DoScroll();
}

void AudacityProject::DoScroll()
{
   const double lowerBound = ScrollingLowerBoundTime();

   int width;
   mTrackPanel->GetTracksUsableArea(&width, NULL);
   mViewInfo.SetBeforeScreenWidth(mViewInfo.sbarH, width, lowerBound);


   if (MayScrollBeyondZero()) {
      enum { SCROLL_PIXEL_TOLERANCE = 10 };
      if (std::abs(mViewInfo.TimeToPosition(0.0, 0
                                   )) < SCROLL_PIXEL_TOLERANCE) {
         // Snap the scrollbar to 0
         mViewInfo.h = 0;
         SetHorizontalThumb(0.0);
      }
   }

   mViewInfo.vpos = mVsbar->GetThumbPosition() * mViewInfo.scrollStep;

   //mchinen: do not always set this project to be the active one.
   //a project may autoscroll while playing in the background
   //I think this is okay since OnMouseEvent has one of these.
   //SetActiveProject(this);

   if (!mAutoScrolling) {
      mTrackPanel->Refresh(false);
   }

   CallAfter(
      [this]{ if (GetTrackPanel())
         GetTrackPanel()->HandleCursorForPresentMouseState(); } );
}

// Utility function called by other zoom methods
void AudacityProject::Zoom(double level)
{
   mViewInfo.SetZoom(level);
   FixScrollbars();
   // See if we can center the selection on screen, and have it actually fit.
   // tOnLeft is the amount of time we would need before the selection left edge to center it.
   float t0 = mViewInfo.selectedRegion.t0();
   float t1 = mViewInfo.selectedRegion.t1();
   float tAvailable = GetScreenEndTime() - mViewInfo.h;
   float tOnLeft = (tAvailable - t0 + t1)/2.0;
   // Bug 1292 (Enh) is effectively a request to do this scrolling of  the selection into view.
   // If tOnLeft is positive, then we have room for the selection, so scroll to it.
   if( tOnLeft >=0 )
      TP_ScrollWindow( t0-tOnLeft);
}

// Utility function called by other zoom methods
void AudacityProject::ZoomBy(double multiplier)
{
   mViewInfo.ZoomBy(multiplier);
   FixScrollbars();
}

// TrackPanel callback method
void AudacityProject::TP_ScrollLeft()
{
   OnScrollLeft();
}

// TrackPanel callback method
void AudacityProject::TP_ScrollRight()
{
   OnScrollRight();
}

// TrackPanel callback method
void AudacityProject::TP_RedrawScrollbars()
{
   FixScrollbars();
}

void AudacityProject::TP_HandleResize()
{
   HandleResize();
}

double AudacityProject::GetZoomOfToFit(){
   const double end = mTracks->GetEndTime();
   const double start = mViewInfo.bScrollBeyondZero
      ? std::min(mTracks->GetStartTime(), 0.0)
      : 0;
   const double len = end - start;

   if (len <= 0.0)
      return mViewInfo.GetZoom();

   int w;
   mTrackPanel->GetTracksUsableArea(&w, NULL);
   w -= 10;
   return w/len;
}

double AudacityProject::GetZoomOfSelection(){
   const double lowerBound =
      std::max(mViewInfo.selectedRegion.t0(), ScrollingLowerBoundTime());
   const double denom =
      mViewInfo.selectedRegion.t1() - lowerBound;
   if (denom <= 0.0)
      return mViewInfo.GetZoom();

   // LL:  The "-1" is just a hack to get around an issue where zooming to
   //      selection doesn't actually get the entire selected region within the
   //      visible area.  This causes a problem with scrolling at end of playback
   //      where the selected region may be scrolled off the left of the screen.
   //      I know this isn't right, but until the real rounding or 1-off issue is
   //      found, this will have to work.
   // PRL:  Did I fix this?  I am not sure, so I leave the hack in place.
   //      Fixes might have resulted from commits
   //      1b8f44d0537d987c59653b11ed75a842b48896ea and
   //      e7c7bb84a966c3b3cc4b3a9717d5f247f25e7296
   int width;
   mTrackPanel->GetTracksUsableArea(&width, NULL);
   return (width - 1) / denom;
}

double AudacityProject::GetZoomOfPreset( int preset ){

   // Sets a limit on how far we will zoom out as a factor over zoom to fit.
   const double maxZoomOutFactor = 4.0;
   // Sets how many pixels we allow for one uint, such as seconds.
   const double pixelsPerUnit = 5.0;

   double result = 1.0;
   double zoomToFit = GetZoomOfToFit();
   switch( preset ){
      default:
      case WaveTrack::kZoomDefault:
         result = ZoomInfo::GetDefaultZoom();
         break;
      case WaveTrack::kZoomToFit:
         result = zoomToFit;
         break;
      case WaveTrack::kZoomToSelection:
         result = GetZoomOfSelection();
         break;
      case WaveTrack::kZoomMinutes:
         result = pixelsPerUnit * 1.0/60;
         break;
      case WaveTrack::kZoomSeconds:
         result = pixelsPerUnit * 1.0;
         break;
      case WaveTrack::kZoom5ths:
         result = pixelsPerUnit * 5.0;
         break;
      case WaveTrack::kZoom10ths:
         result = pixelsPerUnit * 10.0;
         break;
      case WaveTrack::kZoom20ths:
         result = pixelsPerUnit * 20.0;
         break;
      case WaveTrack::kZoom50ths:
         result = pixelsPerUnit * 50.0;
         break;
      case WaveTrack::kZoom100ths:
         result = pixelsPerUnit * 100.0;
         break;
      case WaveTrack::kZoom500ths:
         result = pixelsPerUnit * 500.0;
         break;
      case WaveTrack::kZoomMilliSeconds:
         result = pixelsPerUnit * 1000.0;
         break;
      case WaveTrack::kZoomSamples:
         result = 44100.0;
         break;
      case WaveTrack::kZoom4To1:
         result = 44100.0 * 4;
         break;
      case WaveTrack::kMaxZoom:
         result = ZoomInfo::GetMaxZoom();
         break;
   };
   if( result < (zoomToFit/maxZoomOutFactor) )
      result = zoomToFit / maxZoomOutFactor;
   return result;
}

AudacityProject::PlaybackScroller::PlaybackScroller(AudacityProject *project)
: mProject(project)
{
   mProject->Bind(EVT_TRACK_PANEL_TIMER,
                     &PlaybackScroller::OnTimer,
                     this);
}

void AudacityProject::PlaybackScroller::OnTimer(wxCommandEvent &event)
{
   // Let other listeners get the notification
   event.Skip();

   if(!mProject->IsAudioActive())
      return;
   else if (mMode == Mode::Refresh) {
      // PRL:  see comments in Scrubbing.cpp for why this is sometimes needed.
      // These unnecessary refreshes cause wheel rotation events to be delivered more uniformly
      // to the application, so scrub speed control is smoother.
      // (So I see at least with OS 10.10 and wxWidgets 3.0.2.)
      // Is there another way to ensure that than by refreshing?
      const auto trackPanel = mProject->GetTrackPanel();
      trackPanel->Refresh(false);
   }
   else if (mMode != Mode::Off) {
      // Pan the view, so that we put the play indicator at some fixed
      // fraction of the window width.

      ViewInfo &viewInfo = mProject->GetViewInfo();
      TrackPanel *const trackPanel = mProject->GetTrackPanel();
      const int posX = viewInfo.TimeToPosition(viewInfo.mRecentStreamTime);
      int width;
      trackPanel->GetTracksUsableArea(&width, NULL);
      int deltaX;
      switch (mMode)
      {
         default:
            wxASSERT(false);
            /* fallthru */
         case Mode::Pinned:
            deltaX =
               posX - width * TracksPrefs::GetPinnedHeadPositionPreference();
            break;
         case Mode::Right:
            deltaX = posX - width;        break;
      }
      viewInfo.h =
         viewInfo.OffsetTimeByPixels(viewInfo.h, deltaX, true);
      if (!mProject->MayScrollBeyondZero())
         // Can't scroll too far left
         viewInfo.h = std::max(0.0, viewInfo.h);
      trackPanel->Refresh(false);
   }
}

void AudacityProject::ZoomInByFactor( double ZoomFactor )
{
   // LLL: Handling positioning differently when audio is
   // actively playing.  Don't do this if paused.
   if ((gAudioIO->IsStreamActive(GetAudioIOToken()) != 0) &&
       !gAudioIO->IsPaused()){
      ZoomBy(ZoomFactor);
      mTrackPanel->ScrollIntoView(gAudioIO->GetStreamTime());
      mTrackPanel->Refresh(false);
      return;
   }

   // DMM: Here's my attempt to get logical zooming behavior
   // when there's a selection that's currently at least
   // partially on-screen

   const double endTime = GetScreenEndTime();
   const double duration = endTime - mViewInfo.h;

   bool selectionIsOnscreen =
      (mViewInfo.selectedRegion.t0() < endTime) &&
      (mViewInfo.selectedRegion.t1() >= mViewInfo.h);

   bool selectionFillsScreen =
      (mViewInfo.selectedRegion.t0() < mViewInfo.h) &&
      (mViewInfo.selectedRegion.t1() > endTime);

   if (selectionIsOnscreen && !selectionFillsScreen) {
      // Start with the center of the selection
      double selCenter = (mViewInfo.selectedRegion.t0() +
                          mViewInfo.selectedRegion.t1()) / 2;

      // If the selection center is off-screen, pick the
      // center of the part that is on-screen.
      if (selCenter < mViewInfo.h)
         selCenter = mViewInfo.h +
                     (mViewInfo.selectedRegion.t1() - mViewInfo.h) / 2;
      if (selCenter > endTime)
         selCenter = endTime -
            (endTime - mViewInfo.selectedRegion.t0()) / 2;

      // Zoom in
      ZoomBy(ZoomFactor);
      const double newDuration = GetScreenEndTime() - mViewInfo.h;

      // Recenter on selCenter
      TP_ScrollWindow(selCenter - newDuration / 2);
      return;
   }


   double origLeft = mViewInfo.h;
   double origWidth = duration;
   ZoomBy(ZoomFactor);

   const double newDuration = GetScreenEndTime() - mViewInfo.h;
   double newh = origLeft + (origWidth - newDuration) / 2;

   // MM: Commented this out because it was confusing users
   /*
   // make sure that the *right-hand* end of the selection is
   // no further *left* than 1/3 of the way across the screen
   if (mViewInfo.selectedRegion.t1() < newh + mViewInfo.screen / 3)
      newh = mViewInfo.selectedRegion.t1() - mViewInfo.screen / 3;

   // make sure that the *left-hand* end of the selection is
   // no further *right* than 2/3 of the way across the screen
   if (mViewInfo.selectedRegion.t0() > newh + mViewInfo.screen * 2 / 3)
      newh = mViewInfo.selectedRegion.t0() - mViewInfo.screen * 2 / 3;
   */

   TP_ScrollWindow(newh);
}

void AudacityProject::ZoomOutByFactor( double ZoomFactor )
{
   //Zoom() may change these, so record original values:
   const double origLeft = mViewInfo.h;
   const double origWidth = GetScreenEndTime() - origLeft;

   ZoomBy(ZoomFactor);
   const double newWidth = GetScreenEndTime() - mViewInfo.h;

   const double newh = origLeft + (origWidth - newWidth) / 2;
   // newh = (newh > 0) ? newh : 0;
   TP_ScrollWindow(newh);
}
