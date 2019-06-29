#ifndef __AUDACITY_GONIOMETER__
#define __AUDACITY_GONIOMETER__

#include <memory>
#include "../ClientData.h" // to inherit
#include "../tracks/ui/CommonTrackPanelCell.h" // to inherit
#include "MeterPanelBase.h" // to inherit

class Track;

class Goniometer
   : public std::enable_shared_from_this< Goniometer >
   , public ClientData::Base
   , public CommonTrackPanelCell
   , public Meter
{
public:
   static Goniometer &Get( Track &track );

   explicit Goniometer( Track &track );
   ~Goniometer() override;

   // Meter implementation
   void Clear() override;
   void Reset(double sampleRate, bool resetClipping) override;
   void UpdateDisplay(unsigned numChannels,
                      int numFrames, float *sampleData) override;
   bool IsMeterDisabled() const override;
   bool HasMaxPeak() const override;
   float GetMaxPeak() const override;

   // CommonTrackPanelCell implementation
   std::shared_ptr<Track> DoFindTrack() override;

   // TrackPanelCell implementation
   std::vector<UIHandlePtr> HitTest
      (const TrackPanelMouseState &state,
       const AudacityProject *pProject) override;

private:
   std::vector<float> mRecentSamples;
   size_t mLastSample{ 0 };
   size_t mSampleCount{ 0 };
   size_t mSampleInterval{ 0 };

   Track &mTrack;
};

#endif
