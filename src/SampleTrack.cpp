/**********************************************************************

Audacity: A Digital Audio Editor

SampleTrack.cpp

Paul Licameli split from WaveTrack.cpp

**********************************************************************/

#include "SampleTrack.h"
#include "audacity/Types.h"

SampleTrack::~SampleTrack() = default;

static const Track::TypeInfo &typeInfo()
{
   static const Track::TypeInfo info{
      { "sample", "sample", XO("Sample Track") },
      true, &PlayableTrack::ClassTypeInfo() };
   return info;
}

static TrackTypeRegistry::RegisteredType sType{ "Sample", typeInfo() };

auto SampleTrack::ClassTypeInfo() -> const TypeInfo &
{
   return typeInfo();
}

auto SampleTrack::GetTypeInfo() const -> const TypeInfo &
{
   return typeInfo();
}

sampleCount SampleTrack::TimeToLongSamples(double t0) const
{
   return sampleCount( floor(t0 * GetRate() + 0.5) );
}

double SampleTrack::LongSamplesToTime(sampleCount pos) const
{
   return pos.as_double() / GetRate();
}

EnumSetting< sampleFormat > SampleTrack::FormatSetting{
   wxT("/SamplingRate/DefaultProjectSampleFormatChoice"),
   {
      { wxT("Format16Bit"), XO("16-bit") },
      { wxT("Format24Bit"), XO("24-bit") },
      { wxT("Format32BitFloat"), XO("32-bit float") }
   },
   2, // floatSample

   // for migrating old preferences:
   {
      int16Sample,
      int24Sample,
      floatSample
   },
   wxT("/SamplingRate/DefaultProjectSampleFormat"),
};
