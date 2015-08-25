/**********************************************************************

Audacity: A Digital Audio Editor

TrackPanelCellIterator.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL_CELL_ITERATOR__
#define __AUDACITY_TRACK_PANEL_CELL_ITERATOR__

#include "Track.h"

// This will change to a new abstract base class of Track and of others
class Track;
typedef Track TrackPanelCell;

class TrackPanel;

// A class that allows iteration over the rectangles of visible cells.
class TrackPanelCellIterator
{
public:
   TrackPanelCellIterator(TrackPanel *trackPanel, bool begin);

   // implement the STL iterator idiom

   TrackPanelCellIterator &operator++ ();
   TrackPanelCellIterator operator++ (int);

   friend inline bool operator==
      (const TrackPanelCellIterator &lhs, const TrackPanelCellIterator &rhs)
   {
      return lhs.mpCell == rhs.mpCell;
   }

   typedef std::pair<TrackPanelCell*, wxRect> value_type;
   value_type operator * () const;

private:
   TrackPanel *mPanel;
   VisibleTrackIterator mIter;
   TrackPanelCell *mpCell;
};

inline bool operator !=
(const TrackPanelCellIterator &lhs, const TrackPanelCellIterator &rhs)
{
   return !(lhs == rhs);
}
#endif
