/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackView.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../../Audacity.h" // for USE_* macros
#include "NoteTrackView.h"
#include "NoteTrackVRulerControls.h"

#ifdef USE_MIDI
#include "../../../../NoteTrack.h"

#include "../../../../Experimental.h"

#include "../../../../HitTestResult.h"
#include "../../../../Project.h"
#include "../../../../TrackPanel.h" // for TrackInfo
#include "../../../../TrackPanelMouseEvent.h"
#include "../../../ui/SelectHandle.h"
#include "StretchHandle.h"

NoteTrackView::NoteTrackView( const std::shared_ptr<Track> &pTrack )
   : TrackView{ pTrack }
{
   DoSetHeight( TrackInfo::DefaultNoteTrackHeight() );
}

NoteTrackView::~NoteTrackView()
{
}

NoteTrackView &NoteTrackView::Get( NoteTrack &track )
{
   return *static_cast< NoteTrackView* >( &TrackView::Get( track ) );
}

const NoteTrackView &NoteTrackView::Get( const NoteTrack &track )
{
   return *static_cast< const NoteTrackView* >( &TrackView::Get( track ) );
}

void NoteTrackView::Copy( const TrackView &other )
{
   TrackView::Copy( other );
   
   if ( const auto pOther = dynamic_cast< const NoteTrackView* >( &other ) ) {
      mPitchHeight = pOther->mPitchHeight;
      mBottomNote  = pOther->mBottomNote;
   }
}

void NoteTrackView::Zoom(const wxRect &rect, int y, float multiplier, bool center)
{
   NoteTrackDisplayData data{ *this, rect };
   int clickedPitch = data.YToIPitch(y);
   int extent = mTopNote - mBottomNote + 1;
   int newExtent = (int) (extent / multiplier);
   float position;
   if (center) {
      // center the pitch that the user clicked on
      position = .5;
   } else {
      // align to keep the pitch that the user clicked on in the same place
      position = extent / (clickedPitch - mBottomNote);
   }
   int newBottomNote = clickedPitch - (newExtent * position);
   int newTopNote = clickedPitch + (newExtent * (1 - position));
   SetNoteRange(newBottomNote, newTopNote);
}


void NoteTrackView::ZoomTo(const wxRect &rect, int start, int end)
{
   wxRect trackRect(0, rect.GetY(), 1, rect.GetHeight());
   NoteTrackDisplayData data{ *this, trackRect };
   int pitch1 = data.YToIPitch(start);
   int pitch2 = data.YToIPitch(end);
   if (pitch1 == pitch2) {
      // Just zoom in instead of zooming to show only one note
      Zoom(rect, start, 1, true);
      return;
   }
   // It's fine for this to be in either order
   SetNoteRange(pitch1, pitch2);
}

void NoteTrackView::ZoomAllNotes()
{
   auto &track = *static_cast<NoteTrack*>( FindTrack().get() );
   Alg_iterator iterator( &track.GetSeq(), false );
   iterator.begin();
   Alg_event_ptr evt;

   // Go through all of the notes, finding the minimum and maximum value pitches.
   bool hasNotes = false;
   int minPitch = MaxPitch;
   int maxPitch = MinPitch;

   while (NULL != (evt = iterator.next())) {
      if (evt->is_note()) {
         int pitch = (int) evt->get_pitch();
         hasNotes = true;
         if (pitch < minPitch)
            minPitch = pitch;
         if (pitch > maxPitch)
            maxPitch = pitch;
      }
   }

   if (!hasNotes) {
      // Semi-arbitary default values:
      minPitch = 48;
      maxPitch = 72;
   }

   SetNoteRange(minPitch, maxPitch);
}

std::vector<UIHandlePtr> NoteTrackView::DetailedHitTest
(const TrackPanelMouseState &WXUNUSED(state),
 const AudacityProject *WXUNUSED(pProject), int, bool )
{
   // Eligible for stretch?
   UIHandlePtr result;
   std::vector<UIHandlePtr> results;
#ifdef USE_MIDI
#ifdef EXPERIMENTAL_MIDI_STRETCHING
   result = StretchHandle::HitTest(
      mStretchHandle, state, pProject, Pointer<NoteTrack>(this) );
   if (result)
      results.push_back(result);
#endif
#endif

   return results;
}

std::shared_ptr<TrackVRulerControls> NoteTrackView::DoGetVRulerControls()
{
   return
      std::make_shared<NoteTrackVRulerControls>( shared_from_this() );
}

const float NoteTrackView::ZoomStep = powf( 2.0f, 0.25f );

void NoteTrackView::SetBottomNote(int note)
{
   if (note < MinPitch)
      note = MinPitch;
   else if (note > 96)
      note = 96;

   wxCHECK(note <= mTopNote, );

   mBottomNote = note;
}

void NoteTrackView::SetTopNote(int note)
{
   if (note > MaxPitch)
      note = MaxPitch;

   wxCHECK(note >= mBottomNote, );

   mTopNote = note;
}

void NoteTrackView::SetNoteRange(int note1, int note2)
{
   // Bounds check
   if (note1 > MaxPitch)
      note1 = MaxPitch;
   else if (note1 < MinPitch)
      note1 = MinPitch;
   if (note2 > MaxPitch)
      note2 = MaxPitch;
   else if (note2 < MinPitch)
      note2 = MinPitch;
   // Swap to ensure ordering
   if (note2 < note1) { auto tmp = note1; note1 = note2; note2 = tmp; }

   mBottomNote = note1;
   mTopNote = note2;
}

void NoteTrackView::ShiftNoteRange(int offset)
{
   // Ensure everything stays in bounds
   if (mBottomNote + offset < MinPitch || mTopNote + offset > MaxPitch)
       return;

   mBottomNote += offset;
   mTopNote += offset;
}

NoteTrackDisplayData::NoteTrackDisplayData(const NoteTrackView &view, const wxRect &r)
{
   auto span = view.GetTopNote() - view.GetBottomNote() + 1; // + 1 to make sure it includes both

   mMargin = std::min((int) (r.height / (float)(span)) / 2, r.height / 4);

   // Count the number of dividers between B/C and E/F
   int numC = 0, numF = 0;
   auto botOctave = view.GetBottomNote() / 12, botNote = view.GetBottomNote() % 12;
   auto topOctave = view.GetTopNote() / 12, topNote = view.GetTopNote() % 12;
   if (topOctave == botOctave)
   {
      if (botNote == 0) numC = 1;
      if (topNote <= 5) numF = 1;
   }
   else
   {
      numC = topOctave - botOctave;
      numF = topOctave - botOctave - 1;
      if (botNote == 0) numC++;
      if (botNote <= 5) numF++;
      if (topOctave <= 5) numF++;
   }
   // Effective space, excluding the margins and the lines between some notes
   auto effectiveHeight = r.height - (2 * (mMargin + 1)) - numC - numF;
   // Guarenteed that both the bottom and top notes will be visible
   // (assuming that the clamping below does not happen)
   mPitchHeight = effectiveHeight / ((float) span);

   if (mPitchHeight < MinPitchHeight)
      mPitchHeight = MinPitchHeight;
   if (mPitchHeight > MaxPitchHeight)
      mPitchHeight = MaxPitchHeight;

   mBottom = r.y + r.height - GetNoteMargin() - 1 - GetPitchHeight(1) +
            botOctave * GetOctaveHeight() + GetNotePos(botNote);
}

int NoteTrackDisplayData::IPitchToY(int p) const
{ return mBottom - (p / 12) * GetOctaveHeight() - GetNotePos(p % 12); }

int NoteTrackDisplayData::YToIPitch(int y) const
{
   y = mBottom - y; // pixels above pitch 0
   int octave = (y / GetOctaveHeight());
   y -= octave * GetOctaveHeight();
   // result is approximate because C and G are one pixel taller than
   // mPitchHeight.
   // Poke 1-13-18: However in practice this seems not to be an issue,
   // as long as we use mPitchHeight and not the rounded version
   return (y / mPitchHeight) + octave * 12;
}

#endif
