#include "Goniometer.h"

#include "../AColor.h"
#include "../Track.h"
#include "../TrackArtist.h"
#include "../TrackPanelDrawingContext.h"

#include <wx/dc.h>

namespace{
Track::AttachedObjects::RegisteredFactory sKey{
   []( Track &track ) {
      return std::make_shared<Goniometer>( track );
   }
};
}

Goniometer &Goniometer::Get( Track &track )
{
   return track.AttachedObjects::Get< Goniometer >( sKey );
}

Goniometer::Goniometer( Track &track )
   : mTrack{ track }
{
}

Goniometer::~Goniometer() = default;

void Goniometer::Clear()
{
}

void Goniometer::Reset(double sampleRate, bool resetClipping)
{
}

void Goniometer::UpdateDisplay(unsigned numChannels,
                   int numFrames, float *sampleData)
{
}

bool Goniometer::IsMeterDisabled() const
{
   return false;
}

bool Goniometer::HasMaxPeak() const
{
   return false;
}

float Goniometer::GetMaxPeak() const
{
   return 0.0;
}

std::shared_ptr<Track> Goniometer::DoFindTrack()
{
   return mTrack.SharedPointer();
}

std::vector<UIHandlePtr> Goniometer::HitTest
   (const TrackPanelMouseState &,
    const AudacityProject *)
{
   return {};
}
