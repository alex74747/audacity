/**********************************************************************

Audacity: A Digital Audio Editor

PlaybackScroller.cpp

Paul Licameli split from ProjectWindow.cpp

**********************************************************************/

#include "PlaybackScroller.h"

#include "AudioIO.h"
#include "Project.h"
#include "ProjectAudioIO.h"
#include "ProjectWindow.h"
#include "tracks/ui/Scrubbing.h"
#include "ViewInfo.h"
#include "prefs/TracksPrefs.h"

AudacityProject::AttachedObjects::RegisteredFactory sPlaybackScrollerKey{
   []( AudacityProject &project ){
      return std::make_shared< PlaybackScroller >( project );
   }
};

PlaybackScroller &PlaybackScroller::Get( AudacityProject &project )
{
   return
      project.AttachedObjects::Get< PlaybackScroller >( sPlaybackScrollerKey );
}

const PlaybackScroller &PlaybackScroller::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

bool PlaybackScroller::MayScrollBeyondZero() const
{
   auto &project = mProject;
   auto &scrubber = Scrubber::Get( project );
   auto &viewInfo = ViewInfo::Get( project );
   if (viewInfo.bScrollBeyondZero)
      return true;

   if (scrubber.HasMark() ||
       ProjectAudioIO::Get( project ).IsAudioActive()) {
      auto mode = GetMode();
      if ( mode == Mode::Pinned || mode == Mode::Right )
         return true;
   }

   return false;
}

PlaybackScroller::PlaybackScroller( AudacityProject &project )
: mProject{ project }
{
   mProject.Bind(EVT_TRACK_PANEL_TIMER,
      &PlaybackScroller::OnTimer,
      this);

   ProjectWindow::Get( project )
      .SetMayScrollBeyondZero( [this]{ return MayScrollBeyondZero(); } );
}

void PlaybackScroller::OnTimer(wxCommandEvent &event)
{
   // Let other listeners get the notification
   event.Skip();

   auto cleanup = finally([&]{
      // Propagate the message to other listeners bound to this
      this->ProcessEvent( event );
   });

   if(!ProjectAudioIO::Get( mProject ).IsAudioActive())
      return;
   else if (mMode == Mode::Refresh) {
      // PRL:  see comments in Scrubbing.cpp for why this is sometimes needed.
      // These unnecessary refreshes cause wheel rotation events to be delivered more uniformly
      // to the application, so scrub speed control is smoother.
      // (So I see at least with OS 10.10 and wxWidgets 3.0.2.)
      // Is there another way to ensure that than by refreshing?
      auto &trackPanel = GetProjectPanel( mProject );
      trackPanel.Refresh(false);
   }
   else if (mMode != Mode::Off) {
      // Pan the view, so that we put the play indicator at some fixed
      // fraction of the window width.

      auto gAudioIO = AudioIO::Get();
      auto &viewInfo = ViewInfo::Get( mProject );
      mRecentStreamTime = gAudioIO->GetStreamTime();

      auto &trackPanel = GetProjectPanel( mProject );
      const int posX = viewInfo.TimeToPosition(mRecentStreamTime);
      auto width = viewInfo.GetTracksUsableWidth();
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
      if (!MayScrollBeyondZero())
         // Can't scroll too far left
         viewInfo.h = std::max(0.0, viewInfo.h);
      trackPanel.Refresh(false);
   }
}
