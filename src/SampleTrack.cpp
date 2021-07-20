/**********************************************************************

Audacity: A Digital Audio Editor

SampleTrack.cpp

Paul Licameli split from WaveTrack.cpp

**********************************************************************/

#include "SampleTrack.h"

SampleTrack::~SampleTrack() = default;

static const Track::TypeInfo &typeInfo()
{
   static const Track::TypeInfo info{ TrackKind::Sample,
      { "sample", "sample", XO("Sample Track") },
      true, &PlayableTrack::ClassTypeInfo() };
   return info;
}

auto SampleTrack::ClassTypeInfo() -> const TypeInfo &
{
   return typeInfo();
}

auto SampleTrack::GetTypeInfo() const -> const TypeInfo &
{
   return typeInfo();
}
