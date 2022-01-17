/**********************************************************************

 Audacity: A Digital Audio Editor

 PlayableTrackControls.cpp

 Paul Licameli split from TrackInfo.cpp

 **********************************************************************/

#ifndef __AUDACITY_PLAYABLE_TRACK_CONTROLS__
#define __AUDACITY_PLAYABLE_TRACK_CONTROLS__

#include "CommonTrackControls.h"

class wxRect;
class Track;

class PLAYABLE_TRACK_UI_API PlayableTrackControls /* not final */
   : public CommonTrackControls
{
public:
   // To help subclasses define GetTCPLines
   static const TCPLines& StaticTCPLines();

   static void GetMuteSoloRect(
      const wxRect & rect, wxRect & dest, bool solo, bool bHasSoloButton,
      const Track *pTrack);

   using CommonTrackControls::CommonTrackControls;
};

#endif
