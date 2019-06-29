/**********************************************************************

Audacity: A Digital Audio Editor

ProjectAudioIO.h

Paul Licameli split from AudacityProject.h

**********************************************************************/

#ifndef __PROJECT_AUDIO_IO__
#define __PROJECT_AUDIO_IO__

#include "Observer.h" // to inherit
#include "Project.h"
#include <wx/weakref.h>
#include <wx/event.h> // to declare custom event type

#include <atomic>
#include <memory>
#include <vector>
class AudacityProject;
class Meter;

struct SpeedChangeMessage {};

///\ brief Holds per-project state needed for interaction with AudioIO,
/// including the audio stream token and pointers to meters
class AUDIO_IO_API ProjectAudioIO final
   : public AttachedProjectObject
   , public Observer::Publisher<SpeedChangeMessage>
{
public:
   static ProjectAudioIO &Get( AudacityProject &project );
   static const ProjectAudioIO &Get( const AudacityProject &project );

   explicit ProjectAudioIO( AudacityProject &project );
   ProjectAudioIO( const ProjectAudioIO & ) = delete;
   ProjectAudioIO &operator=( const ProjectAudioIO & ) = delete;
   ~ProjectAudioIO();

   int GetAudioIOToken() const;
   bool IsAudioActive() const;
   void SetAudioIOToken(int token);

   using MeterPtr = std::shared_ptr< Meter >;
   using Meters = std::vector< MeterPtr >;

   const Meters &GetPlaybackMeters() const;
   bool HasPlaybackMeter( Meter *pMeter ) const;
   void AddPlaybackMeter(const MeterPtr &playback);
   void RemovePlaybackMeter(Meter *playback);

   const Meters &GetCaptureMeters() const;
   bool HasCaptureMeter( Meter *pMeter ) const;
   void AddCaptureMeter(const MeterPtr &capture);
   void RemoveCaptureMeter(Meter *capture);

   // Speed play
   double GetPlaySpeed() const {
      return mPlaySpeed.load( std::memory_order_relaxed ); }
   void SetPlaySpeed( double value );

private:
   AudacityProject &mProject;

   // Project owned meters
   Meters mPlaybackMeters;
   Meters mCaptureMeters;

   // This is atomic because scrubber may read it in a separate thread from
   // the main
   std::atomic<double> mPlaySpeed{};

   int  mAudioIOToken{ -1 };
};

#endif
