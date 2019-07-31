/**********************************************************************

Audacity: A Digital Audio Editor

PlaybackScroller.h

Paul Licameli split from ProjectWindow.h

**********************************************************************/

#ifndef __AUDACITY_PLAYBACK_SCROLLER__
#define __AUDACITY_PLAYBACK_SCROLLER__

#include "ClientData.h" // to inherit

class AudacityProject;

class PlaybackScroller final
   : public wxEvtHandler
   , public ClientData::Base
{
public:
   static PlaybackScroller &Get( AudacityProject &project );
   static const PlaybackScroller &Get( const AudacityProject &project );

   explicit PlaybackScroller( AudacityProject &project );

   PlaybackScroller( const PlaybackScroller & ) = delete;
   PlaybackScroller& operator=( const PlaybackScroller & ) = delete;

   enum class Mode {
      Off,
      Refresh,
      Pinned,
      Right,
   };

   Mode GetMode() const { return mMode; }
   void Activate(Mode mode)
   {
      mMode = mode;
   }

   double GetRecentStreamTime() const { return mRecentStreamTime; }

   bool MayScrollBeyondZero() const;

private:
   void OnTimer(wxCommandEvent &event);

   AudacityProject &mProject;
   Mode mMode { Mode::Off };

   // During timer update, grab the volatile stream time just once, so that
   // various other drawing code can use the exact same value.
   double mRecentStreamTime{ -1.0 };
};

#endif
