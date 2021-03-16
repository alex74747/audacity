/**********************************************************************

  Audacity: A Digital Audio Editor

  @file TrackArt.h

  Paul Licameli split from TrackArtist.h

**********************************************************************/
#ifndef __AUDACITY_TRACK_ART__
#define __AUDACITY_TRACK_ART__

#include "Audacity.h"

class Track;
struct TrackPanelDrawingContext;
class wxBrush;
class wxRect;

namespace TrackArt {

   // Helper: draws the "sync-locked" watermark tiled to a rectangle
   AUDACITY_DLL_API
   void DrawSyncLockTiles(
      TrackPanelDrawingContext &context, const wxRect &rect );

   // Helper: draws background with selection rect
   AUDACITY_DLL_API
   void DrawBackgroundWithSelection(TrackPanelDrawingContext &context,
         const wxRect &rect, const Track *track,
         const wxBrush &selBrush, const wxBrush &unselBrush,
         bool useSelection = true);

   AUDACITY_DLL_API
   void DrawNegativeOffsetTrackArrows( TrackPanelDrawingContext &context,
                                       const wxRect & rect );
}

extern AUDACITY_DLL_API int GetWaveYPos(float value, float min, float max,
                       int height, bool dB, bool outer, float dBr,
                       bool clip);
extern float FromDB(float value, double dBRange);
extern AUDACITY_DLL_API float ValueOfPixel(int yy, int height, bool offset,
                          bool dB, double dBRange, float zoomMin, float zoomMax);

#endif
