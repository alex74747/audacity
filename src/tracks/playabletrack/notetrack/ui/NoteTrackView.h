/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackView.h

Paul Licameli split from class NoteTrack

**********************************************************************/

#ifndef __AUDACITY_NOTE_TRACK_VIEW__
#define __AUDACITY_NOTE_TRACK_VIEW__

#include "../../../ui/TrackView.h"

class NoteTrack;

class NoteTrackView final : public TrackView
{
   NoteTrackView( const NoteTrackView& ) = delete;
   NoteTrackView &operator=( const NoteTrackView& ) = delete;

public:
   explicit
   NoteTrackView( const std::shared_ptr<Track> &pTrack );
   ~NoteTrackView() override;

   static NoteTrackView &Get( NoteTrack & );
   static const NoteTrackView &Get( const NoteTrack & );

   std::shared_ptr<TrackVRulerControls> DoGetVRulerControls() override;

   /// Gets the current bottom note (a pitch)
   int GetBottomNote() const { return mBottomNote; }
   /// Gets the current top note (a pitch)
   int GetTopNote() const { return mTopNote; }
   /// Sets the bottom note (a pitch), making sure that it is never greater than the top note.
   void SetBottomNote(int note);
   /// Sets the top note (a pitch), making sure that it is never less than the bottom note.
   void SetTopNote(int note);
   /// Sets the top and bottom note (both pitches) automatically, swapping them if needed.
   void SetNoteRange(int note1, int note2);

   /// Zooms so that all notes are visible
   void ZoomAllNotes();
   /// Zooms so that the entire track is visible
   void ZoomMaxExtent() { SetNoteRange(MinPitch, MaxPitch); }
   /// Shifts all notes vertically by the given pitch
   void ShiftNoteRange(int offset);

   /// Zooms out a constant factor (subject to zoom limits)
   void ZoomOut(const wxRect &rect, int y)
      { Zoom(rect, y, 1.0f / ZoomStep, true); }

   /// Zooms in a contant factor (subject to zoom limits)
   void ZoomIn(const wxRect &rect, int y) { Zoom(rect, y, ZoomStep, true); }

   /// Zoom the note track around y.
   /// If center is true, the result will be centered at y.
   void Zoom(const wxRect &rect, int y, float multiplier, bool center);

   void ZoomTo(const wxRect &rect, int start, int end);

private:
   // Preserve some view state too for undo/redo purposes
   void Copy( const TrackView &other ) override;

   std::vector<UIHandlePtr> DetailedHitTest
      (const TrackPanelMouseState &state,
       const AudacityProject *pProject, int currentTool, bool bMultiTool)
      override;

   int mBottomNote{ MinPitch }, mTopNote{ MaxPitch };
#if 0
   // Also unused from vertical scrolling
   int mStartBottomNote;
#endif

   // Remember continuous variation for zooming,
   // but it is rounded off whenever drawing:
   float mPitchHeight{ 5.0f };

   enum { MinPitch = 0, MaxPitch = 127 };

   static const float ZoomStep;
};

/// Data used to display a note track
class NoteTrackDisplayData {
private:
   float mPitchHeight;
   // mBottom is the Y offset of pitch 0 (normally off screen)
   // Used so that mBottomNote is located at
   // mY + mHeight - (GetNoteMargin() + 1 + GetPitchHeight())
   int mBottom;
   int mMargin;

   enum { MinPitchHeight = 1, MaxPitchHeight = 25 };
public:
   NoteTrackDisplayData(const NoteTrackView &view, const wxRect &r);

   int GetPitchHeight(int factor) const
   { return std::max(1, (int)(factor * mPitchHeight)); }
   int GetNoteMargin() const { return mMargin; };
   int GetOctaveHeight() const { return GetPitchHeight(12) + 2; }
   // IPitchToY returns Y coordinate of top of pitch p
   int IPitchToY(int p) const;
   // compute the window coordinate of the bottom of an octave: This is
   // the bottom of the line separating B and C.
   int GetOctaveBottom(int oct) const {
      return IPitchToY(oct * 12) + GetPitchHeight(1) + 1;
   }
   // Y coordinate for given floating point pitch (rounded to int)
   int PitchToY(double p) const {
      return IPitchToY((int) (p + 0.5));
   }
   // Integer pitch corresponding to a Y coordinate
   int YToIPitch(int y) const;
   // map pitch class number (0-11) to pixel offset from bottom of octave
   // (the bottom of the black line between B and C) to the top of the
   // note. Note extra pixel separates B(11)/C(0) and E(4)/F(5).
   int GetNotePos(int p) const
   { return 1 + GetPitchHeight(p + 1) + (p > 4); }
   // get pixel offset to top of ith black key note
   int GetBlackPos(int i) const { return GetNotePos(i * 2 + 1 + (i > 1)); }
   // GetWhitePos tells where to draw lines between keys as an offset from
   // GetOctaveBottom. GetWhitePos(0) returns 1, which matches the location
   // of the line separating B and C
   int GetWhitePos(int i) const { return 1 + (i * GetOctaveHeight()) / 7; }
};

#endif
