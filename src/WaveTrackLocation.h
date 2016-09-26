/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackLocation.h

Paul Licameli -- split from WaveTrack.h

**********************************************************************/

#ifndef __AUDACITY_WAVE_TRACK_LOCATION__
#define __AUDACITY_WAVE_TRACK_LOCATION__

struct WaveTrackLocation {

   enum LocationType {
      locationCutLine = 1,
      locationMergePoint
   };

   explicit
   WaveTrackLocation
      (double pos_, size_t clipidx1_, size_t clipidx2_)
      : pos(pos_), typ(locationMergePoint), clipidx1(clipidx1_), clipidx2(clipidx2_)
   {}

   explicit
   WaveTrackLocation
      (double pos_ = 0.0)
      : pos(pos_), typ(locationCutLine)
   {}

   // Position of track location
   double pos;

   // Type of track location
   LocationType typ;

   // Only for typ==locationMergePoint
   size_t clipidx1 {}; // first clip (left one)
   size_t clipidx2 {}; // second clip (right one)
};

#endif
