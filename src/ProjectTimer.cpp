/**********************************************************************

Audacity: A Digital Audio Editor

ProjectTimer.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

// no need yet for a separate .h file

#include <wx/timer.h> // to inherit
#include <wx/eventfilter.h> // to inherit
#include "ClientData.h" // to inherit

class AudacityProject;

class AUDACITY_DLL_API ProjectTimer final
   : public wxTimer
   , public wxEventFilter
   , public ClientData::Base
{
public:

   explicit ProjectTimer( AudacityProject &project );
   ~ProjectTimer() override;

   ProjectTimer( const ProjectTimer& ) = delete;
   ProjectTimer &operator=( const ProjectTimer & ) = delete;

   void Notify() override;

   int FilterEvent(wxEvent &event) override;

private:
   void OnIdle(wxIdleEvent & event);

   AudacityProject &mProject;
   int mTimeCount{ 0 };
};

#include "AudioIO.h"
#include "Project.h"
#include "ProjectAudioIO.h"
#include "ProjectAudioManager.h"
#include "ProjectWindow.h"
#include "TrackPanel.h"

namespace {

AudacityProject::AttachedObjects::RegisteredFactory sTimerKey{
   []( AudacityProject &project ){
      return std::make_shared< ProjectTimer >( project ); }
};

}

ProjectTimer::ProjectTimer( AudacityProject &project )
   : mProject{ project }
{
   // Timer is started after the window is visible
   ProjectWindow::Get( mProject )
      .Bind( wxEVT_IDLE, &ProjectTimer::OnIdle, this );
   wxEvtHandler::AddFilter( this );
}

ProjectTimer::~ProjectTimer()
{
   wxEvtHandler::RemoveFilter( this );
}

void ProjectTimer::OnIdle( wxIdleEvent &event )
{
   event.Skip();
   auto &window = TrackPanel::Get( mProject );
   // The window must be ready when the timer fires (#1401)
   if (window.IsShownOnScreen())
   {
      Start(kTimerInterval, FALSE);

      // Timer is started, we don't need the event anymore
      GetProjectFrame( mProject ).Unbind(wxEVT_IDLE,
         &ProjectTimer::OnIdle, this);
   }
   else
   {
      // Get another idle event, wx only guarantees we get one
      // event after "some other normal events occur"
      event.RequestMore();
   }
}

void ProjectTimer::Notify()
{
   mTimeCount++;

   AudacityProject *const p = &mProject;
   auto &trackPanel = TrackPanel::Get( *p );
   auto &window = ProjectWindow::Get( *p );
   
   auto &projectAudioIO = ProjectAudioIO::Get( *p );
   auto gAudioIO = AudioIO::Get();
   
   // Check whether we were playing or recording, but the stream has stopped.
   if (projectAudioIO.GetAudioIOToken()>0 &&
      !projectAudioIO.IsAudioActive())
   {
      //the stream may have been started up after this one finished (by some other project)
      //in that case reset the buttons don't stop the stream
      auto &projectAudioManager = ProjectAudioManager::Get( *p );
      projectAudioManager.Stop(!gAudioIO->IsStreamActive());
   }
   
   // Next, check to see if we were playing or recording
   // audio, but now Audio I/O is completely finished.
   if (projectAudioIO.GetAudioIOToken()>0 &&
       !gAudioIO->IsAudioTokenActive(projectAudioIO.GetAudioIOToken()))
   {
      projectAudioIO.SetAudioIOToken(0);
      window.RedrawProject();
   }

   // Notify listeners for timer ticks
   // (From Debian)
   //
   // Don't call TrackPanel::OnTimer(..) directly here, but instead post
   // an event. This ensures that this is a pure wxWidgets event
   // (no GDK event behind it) and that it therefore isn't processed
   // within the YieldFor(..) of the clipboard operations (workaround
   // for Debian bug #765341).
   // QueueEvent() will take ownership of the event
   p->QueueEvent( safenew wxCommandEvent{ EVT_PROJECT_TIMER } );

   if(projectAudioIO.IsAudioActive() && gAudioIO->GetNumCaptureChannels()) {

      // Periodically update the display while recording

      if ((mTimeCount % 5) == 0) {
         // Must tell OnPaint() to recreate the backing bitmap
         // since we've not done a full refresh.
         trackPanel.RefreshBacking();
         trackPanel.Refresh( false );
      }
   }
   if(mTimeCount > 1000)
      mTimeCount = 0;
}

int ProjectTimer::FilterEvent(wxEvent &event)
{
   if ( event.GetEventType() == wxEVT_LEFT_DOWN ) {
      // wxTimers seem to be a little unreliable, so this
      // "primes" it to make sure it keeps going for a while...

      // When this timer fires, we call TrackPanel::OnTimer and
      // possibly update the screen for offscreen scrolling.
      Stop();
      Start(kTimerInterval, FALSE);
   }
   return Event_Skip;
}
