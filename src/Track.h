/**********************************************************************

  Audacity: A Digital Audio Editor

  Track.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_TRACK__
#define __AUDACITY_TRACK__

#include "Audacity.h"

#include "MemoryX.h"
#include <vector>
#include <list>
#include <wx/dynarray.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/longlong.h>
#include <wx/string.h>

#include "Experimental.h"
#include "SampleFormat.h"
#include "xml/XMLTagHandler.h"

#ifdef __WXMSW__
#pragma warning(disable:4284)
#endif

class wxTextFile;
class DirManager;
class Track;
class LabelTrack;
class TimeTrack;
class WaveTrack;
class NoteTrack;
class AudacityProject;
class ZoomInfo;

WX_DEFINE_USER_EXPORTED_ARRAY(Track*, TrackArray, class AUDACITY_DLL_API);
using WaveTrackArray = std::vector < WaveTrack* > ;

class WaveTrackConstArray : public std::vector < const WaveTrack* > {
public:
   WaveTrackConstArray() {}
   // I'd like to use an inherited constructor, but that's not here yet in MSVC compiler...
#ifdef __AUDACITY_OLD_STD__
   WaveTrackConstArray
      (std::initializer_list<value_type> tracks) 
   {
      reserve(tracks.size());
      for (const auto &track : tracks)
         push_back(track);
   }
#else
   WaveTrackConstArray
      (std::initializer_list<value_type> tracks) : std::vector<value_type>(tracks) {}
#endif
};

using NoteTrackArray  = std::vector < NoteTrack* >;

#if defined(USE_MIDI)
class NoteTrack;
#endif

class TrackList;

using ListOfTracks = std::list<movable_ptr<Track>>;

using TrackNodePointer = ListOfTracks::iterator;

enum class TrackKind
{
   None,
   Wave,
#if defined(USE_MIDI)
   Note,
#endif
   Label,
   Time,
   All
};

// This bit of metaprogramming lets track_cast work even when the track
// subclasses are visible only as incomplete types
template<typename T> struct track_traits {};

template<> struct track_traits< Track * >
{ static const TrackKind kind = TrackKind::None;
   using Result = Track *;
};
template<> struct track_traits< LabelTrack * >
{ static const TrackKind kind = TrackKind::Label;
   using Result = LabelTrack *;
};
template<> struct track_traits< NoteTrack * >
{ static const TrackKind kind = TrackKind::Note;
   using Result = NoteTrack *;
};
template<> struct track_traits< TimeTrack * >
{ static const TrackKind kind = TrackKind::Time;
   using Result = TimeTrack *;
};
template<> struct track_traits< WaveTrack * >
{ static const TrackKind kind = TrackKind::Wave;
   using Result = WaveTrack *;
};

template<> struct track_traits< const Track * >
{ static const TrackKind kind = TrackKind::None;
   using Const_Result = const Track *;
};
template<> struct track_traits< const LabelTrack * >
{ static const TrackKind kind = TrackKind::Label;
   using Const_Result = const LabelTrack *;
};
template<> struct track_traits< const NoteTrack * >
{ static const TrackKind kind = TrackKind::Note;
   using Const_Result = const NoteTrack *;
};
template<> struct track_traits< const TimeTrack * >
{ static const TrackKind kind = TrackKind::Time;
   using Const_Result = const TimeTrack *;
};
template<> struct track_traits< const WaveTrack * >
{ static const TrackKind kind = TrackKind::Wave;
   using Const_Result = const WaveTrack *;
};

// forward declarations, so we can make them friends
template<typename T>
inline typename track_traits<T>::Result track_cast(Track *track);

template<typename T>
inline typename track_traits<T>::Const_Result track_cast(Track *track);

template<typename T>
inline typename track_traits<T>::Const_Result track_cast(const Track *track);

class AUDACITY_DLL_API Track /* not final */ : public XMLTagHandler
{
   friend class TrackList;
   friend class TrackListIterator;
   friend class SyncLockedTracksIterator;

 // To be TrackDisplay
 protected:
   TrackList     *mList;
   TrackNodePointer mNode{};
   int            mIndex;
   int            mY;
   int            mHeight;
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   int            mYv;   //For mono a virtual Y value is necessary.
   int            mHeightv; // For mono a virtual height value is necessary.
   float          mPerY; //mY as a percent of mYv + mY
   bool           mVirtualStereo;
#endif
   wxString       mName;
   wxString       mDefaultName;

   bool           mSelected;

   bool           mLinked;
   bool           mMinimized;

 public:
   mutable wxSize vrulerSize;

   // This just returns a constant and can be overriden by subclasses
   // to specify a different height for the case that the track is minimized.
   virtual int GetMinimizedHeight() const;
   int GetActualHeight() { return mHeight; };

   int GetIndex() const;
   void SetIndex(int index);
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   int GetY(bool vStereo = false) const;
   void SetY(int y, bool vStereo = false);
   int GetHeight(bool vStereo = false) const;
   void SetHeight(int h, bool vStereo = false);
#else
   int GetY() const;
   void SetY(int y);
   int GetHeight() const;
   void SetHeight(int h);
#endif
   bool GetMinimized() const;
   void SetMinimized(bool isMinimized);
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   float GetVirtualTrackPercentage() { return mPerY;}
   void SetVirtualTrackPercentage(float val) { mPerY = val;}
   bool GetVirtualStereo() { return mVirtualStereo;}
   void SetVirtualStereo(bool vStereo) { mVirtualStereo = vStereo;}
#endif
   Track *GetLink() const;

 private:
   TrackNodePointer GetNode() const;
   void SetOwner(TrackList *list, TrackNodePointer node);

 // Keep in Track

 protected:
   int                 mChannel;
   double              mOffset;
   bool                mMute;
   bool                mSolo;

   mutable std::shared_ptr<DirManager> mDirManager;

 public:
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   void ReorderList(bool resize = true);
#endif

   enum
   {
      LeftChannel = 0,
      RightChannel = 1,
      MonoChannel = 2
   };

   Track(const std::shared_ptr<DirManager> &projDirManager);
   Track(const Track &orig);

   virtual ~ Track();

   void Init(const Track &orig);

   using Holder = std::unique_ptr<Track>;
   virtual Holder Duplicate() const = 0;

   // Called when this track is merged to stereo with another, and should
   // take on some paramaters of its partner.
   virtual void Merge(const Track &orig);

   wxString GetName() const { return mName; }
   void SetName( const wxString &n ) { mName = n; }
   wxString GetDefaultName() const { return mDefaultName; }
   void SetDefaultName( const wxString &n ) { mDefaultName = n; }

   bool GetSelected() const { return mSelected; }
   bool GetMute    () const { return mMute;     }
   bool GetLinked  () const { return mLinked;   }
   bool GetSolo    () const { return mSolo;     }

   virtual void SetSelected(bool s);
   void SetMute    (bool m) { mMute     = m; }
   void SetLinked  (bool l);
   void SetSolo    (bool s) { mSolo     = s; }

   int    GetChannel() const { return mChannel; }
   virtual double GetOffset() const = 0;

   void Offset(double t) { SetOffset(GetOffset() + t); }
   virtual void SetOffset (double o) { mOffset = o; }

   void SetChannel(int    c) { mChannel = c; }

   // AS: Note that the dirManager is mutable.  This is
   // mostly to support "Duplicate" of const objects,
   // but in general, mucking with the dir manager is
   // separate from the Track.
   const std::shared_ptr<DirManager> &GetDirManager() const { return mDirManager; }

   // Create a NEW track and modify this track (or return null for failure)
   virtual Holder Cut(double WXUNUSED(t0), double WXUNUSED(t1)) { return{}; }

   // Create a NEW track and don't modify this track (or return null for failure)
   virtual Holder Copy(double WXUNUSED(t0), double WXUNUSED(t1)) const { return{}; }

   // Return true for success
   virtual bool Clear(double WXUNUSED(t0), double WXUNUSED(t1)) {return false;}

   // Return true for success
   virtual bool Paste(double WXUNUSED(t), const Track * WXUNUSED(src)) {return false;}

   // This can be used to adjust a sync-lock selected track when the selection
   // is replaced by one of a different length.
   virtual bool SyncLockAdjust(double oldT1, double newT1);

   virtual bool Silence(double WXUNUSED(t0), double WXUNUSED(t1)) {return false;}
   virtual bool InsertSilence(double WXUNUSED(t), double WXUNUSED(len)) {return false;}

private:
   virtual TrackKind GetKind() const { return TrackKind::None; }
   template<typename T>
   friend typename track_traits<T>::Result track_cast(Track *track);
   template<typename T>
   friend typename track_traits<T>::Const_Result track_cast(Track *track);
   template<typename T>
   friend typename track_traits<T>::Const_Result track_cast(const Track *track);
   friend class TrackListOfKindIterator;

public:
   bool SameKindAs(const Track &track) const
      { return GetKind() == track.GetKind(); }

   // XMLTagHandler callback methods -- NEW virtual for writing
   virtual void WriteXML(XMLWriter &xmlFile) = 0;

   // Returns true if an error was encountered while trying to
   // open the track from XML
   virtual bool GetErrorOpening() { return false; }

   virtual double GetStartTime() const = 0;
   virtual double GetEndTime() const = 0;

   // Checks if sync-lock is on and any track in its sync-lock group is selected.
   bool IsSyncLockSelected() const;
};

// Functions to encapsulate the checked down-casting of track pointers,
// eliminating possibility of error -- and not quietly casting away const
// typical usage:
// if (auto wt = track_cast<WaveTrack*>(track)) { ... }
template<typename T>
inline typename track_traits<T>::Result
track_cast(Track *track)
{
   if (track && track_traits<T>::kind == track->GetKind())
      return (typename track_traits<T>::Result)(track);
   else
      return nullptr;
}

template<typename T>
inline typename track_traits<T>::Const_Result
track_cast(Track *track)
{
   if (track && track_traits<T>::kind == track->GetKind())
      return (typename track_traits<T>::Const_Result)(track);
   else
      return nullptr;
}

// Overload for const pointers can cast only to other const pointer types
template<typename T>
inline typename track_traits<T>::Const_Result
track_cast(const Track *track)
{
   if (track && track_traits<T>::kind == track->GetKind())
      return (typename track_traits<T>::Const_Result)(track);
   else
      return nullptr;
}

// new track iterators can eliminate the need to cast the result
template <
   typename TrackType, // Track or a subclass, maybe const-qualified
   typename Pred // copyable, takes const reference to TrackType
> class TrackIter
   : public std::iterator< std::bidirectional_iterator_tag, TrackType * >
{
public:
   TrackIter( TrackNodePointer begin, TrackNodePointer iter,
              TrackNodePointer end, Pred pred )
      : mBegin( begin ), mIter( iter ), mEnd( end ), mPred( pred )
   {
      if (mIter != mEnd && !valid())
         operator ++ ();
   }

   TrackIter &operator ++ ()
   {
      do
         ++mIter;
      while (mIter != mEnd && !valid() );
      return *this;
   }

   TrackIter operator ++ (int)
   {
      TrackIter result { *this };
      operator ++ ();
      return result;
   }

   TrackIter &operator -- ()
   {
      do {
         if (mIter == mBegin)
            // Go circularly
            mIter = mEnd;
         else
            --mIter;
      } while (mIter != mEnd && !valid() );
      return *this;
   }

   TrackIter operator -- (int)
   {
      TrackIter result { *this };
      operator -- ();
      return result;
   }

   TrackType *operator * () const
   {
      if (mIter == mEnd)
         return nullptr;
      else
         // Other methods guarantee that the cast is correct
         return static_cast< TrackType * >( &**mIter );
   }

   friend inline bool operator == (TrackIter a, TrackIter b)
   {
      return
         a.mIter == b.mIter &&
         a.mBegin == b.mBegin &&
         a.mEnd == b.mEnd;
   }

   friend inline bool operator != (TrackIter a, TrackIter b)
   {
      return !(a == b);
   }

private:
   bool valid() const
   {
      // assume mIter != mEnd
      TrackType *pTrack = track_cast< TrackType * >( &**mIter );
      if (!pTrack)
         return false;
      const TrackType &track = *pTrack;
      return mPred( track );
   }

   TrackNodePointer mIter, mBegin, mEnd;
   Pred mPred;
};

class AUDACITY_DLL_API TrackListIterator /* not final */
{
 public:
   TrackListIterator(TrackList * val = NULL);
   virtual ~TrackListIterator() {}

   // Iterate functions
   virtual Track *First(TrackList * val = NULL);
   virtual Track *StartWith(Track * val);
   virtual Track *Next(bool skiplinked = false);
   virtual Track *Prev(bool skiplinked = false);
   virtual Track *Last(bool skiplinked = false);

   Track *RemoveCurrent(); // deletes track, returns next

 protected:
   friend TrackList;

   TrackList *l;
   TrackNodePointer cur{};
};

class AUDACITY_DLL_API TrackListConstIterator
{
public:
   TrackListConstIterator(const TrackList * val = NULL)
      : mIter(const_cast<TrackList*>(val))
   {}
   ~TrackListConstIterator() {}

   // Iterate functions
   const Track *First(const TrackList * val = NULL)
   { return mIter.First(const_cast<TrackList*>(val)); }
   const Track *StartWith(const Track * val)
   { return mIter.StartWith(const_cast<Track*>(val)); }
   const Track *Next(bool skiplinked = false)
   { return mIter.Next(skiplinked); }
   const Track *Prev(bool skiplinked = false)
   { return mIter.Prev(skiplinked); }
   const Track *Last(bool skiplinked = false)
   { return mIter.Last(skiplinked); }

private:
   TrackListIterator mIter;
};

// TrackListCondIterator (base class for iterators that iterate over all tracks)
// that meet a condition)
class AUDACITY_DLL_API TrackListCondIterator /* not final */ : public TrackListIterator
{
   public:
      TrackListCondIterator(TrackList *val = NULL)
         :  TrackListIterator(val) {}
      virtual ~TrackListCondIterator() {}

      // Iteration functions
      Track *First(TrackList *val = NULL) override;
      Track *StartWith(Track *val) override;
      Track *Next(bool skiplinked = false) override;
      Track *Prev(bool skiplinked = false) override;
      Track *Last(bool skiplinked = false) override;

   protected:
      // NEW virtual
      virtual bool Condition(Track *t) = 0;
};

//
// TrackListOfKindIterator
//
// Based on TrackListIterator and returns only tracks of the specified type.
//
class AUDACITY_DLL_API TrackListOfKindIterator /* not final */ : public TrackListCondIterator
{
 public:
   TrackListOfKindIterator(TrackKind kind, TrackList * val = nullptr);
   virtual ~TrackListOfKindIterator() {}

 protected:
   virtual bool Condition(Track *t) override;

 private:
   TrackKind kind;
};

//
// SelectedTrackListOfKindIterator
//
// Based on TrackListOfKindIterator and returns only tracks selected.
//
class AUDACITY_DLL_API SelectedTrackListOfKindIterator final : public TrackListOfKindIterator
{
 public:
   SelectedTrackListOfKindIterator(TrackKind kind, TrackList * val = NULL) : TrackListOfKindIterator(kind, val) {}
   virtual ~SelectedTrackListOfKindIterator() {}

 protected:
   bool Condition(Track *t) override;
};

//
// VisibleTrackIterator
//
// Based on TrackListIterator returns only the currently visible tracks.
//
class AUDACITY_DLL_API VisibleTrackIterator final : public TrackListCondIterator
{
 public:
   VisibleTrackIterator(AudacityProject *project);
   virtual ~VisibleTrackIterator() {}

 protected:
   bool Condition(Track *t) override;

 private:
   AudacityProject *mProject;
   wxRect mPanelRect;
};


// SyncLockedTracksIterator returns only tracks belonging to the sync-locked tracks
// in which the starting track is a member.
class AUDACITY_DLL_API SyncLockedTracksIterator final : public TrackListIterator
{
 public:
   SyncLockedTracksIterator(TrackList * val);
   virtual ~SyncLockedTracksIterator() {}

   // Iterate functions
   Track *StartWith(Track *member) override;
   Track *Next(bool skiplinked = false) override;
   Track *Prev(bool skiplinked = false) override;
   Track *Last(bool skiplinked = false) override;

 private:
   bool mInLabelSection;
};


/** \brief TrackList is a flat linked list of tracks supporting Add,  Remove,
 * Clear, and Contains, plus serialization of the list of tracks.
 */

// Posted when the horizontal positions within tracks have beed updated.  The
// wxCommandEvent::GetClientData() method can be used to retrieve the first
// track that was updated.  All positions following that track will have been
// updated as well.
DECLARE_EXPORTED_EVENT_TYPE(AUDACITY_DLL_API, EVT_TRACKLIST_RESIZED, -1);

// Posted when tracks have been added or deleted from a tracklist.  The pointer
// wxCommandEvent::GetClientData() returns will be NULL for deletions or the
// track that was added.
DECLARE_EXPORTED_EVENT_TYPE(AUDACITY_DLL_API, EVT_TRACKLIST_UPDATED, -1);

class TrackList final : public wxEvtHandler, public ListOfTracks
{
 public:
   // Create an empty TrackList
   TrackList();

   // Allow copy -- a deep copy that duplicates all tracks
   TrackList(const TrackList &that);
   TrackList &operator= (const TrackList &that);

   // Allow move
   TrackList(TrackList &&that);
   TrackList& operator= (TrackList&&);

   // Move is defined in terms of Swap
   void Swap(TrackList &that);


   // Destructor
   virtual ~TrackList();

   // Iteration
private:
   static inline bool TruePred (const Track &)
      { return true; }
   static inline bool SelectedPred (const Track &track)
      { return track.GetSelected(); }

public:
   template < typename TrackType = Track >
      IteratorRange< TrackIter< TrackType, decltype(&TruePred) > >
   Tracks()
   {
      auto pred = &TruePred;
      auto b = begin(), e = end();
      return { { b, b, e, pred }, { b, e, e, pred } };
   }

   template < typename TrackType = Track, typename Pred >
      IteratorRange< TrackIter< TrackType, Pred > >
         Tracks( const Pred &pred )
   {
      auto b = begin(), e = end();
      return { { b, b, e, pred }, { b, e, e, pred } };
   }

   template < typename TrackType = const Track >
      IteratorRange< TrackIter< TrackType, decltype(&TruePred) > >
         Tracks() const
   {
      // const access:  require TrackType to be const
      using std::
#ifdef __AUDACITY_OLD_STD__
         tr1::
#endif
         is_const;
      static_assert
         ( is_const< TrackType >::value,
           "const-qualified track subtype is required" );

      auto pred = &TruePred;
      auto b = begin(), e = end();
      return { { b, b, e, pred }, { b, e, e, pred } };
   }

   template < typename TrackType = const Track, typename Pred >
      IteratorRange< TrackIter< TrackType, Pred > >
         Tracks( const Pred &pred = &TruePred ) const
   {
      // const access:  require TrackType to be const
      using std::
#ifdef __AUDACITY_OLD_STD__
         tr1::
#endif
         is_const;
      static_assert
         ( is_const< TrackType >::value,
           "const-qualified track subtype is required" );

      auto b = begin(), e = end();
      return { { b, b, e, pred }, { b, e, e, pred } };
   }

   template < typename TrackType = Track >
      IteratorRange< TrackIter< TrackType, decltype(&SelectedPred) > >
         SelectedTracks()
   {
      return Tracks< TrackType >( &SelectedPred );
   }

   template < typename TrackType = Track >
      IteratorRange< TrackIter< const TrackType, decltype(&SelectedPred) > >
         SelectedTracks() const
   {
      return Tracks< TrackType >( &SelectedPred );
   }

   friend class Track;
   friend class TrackListIterator;
   friend class SyncLockedTracksIterator;

   /// For use in sorting:  assume each iterator points into this list, no duplications
   void Permute(const std::vector<TrackNodePointer> &permutation);

   /// Add this Track or all children of this TrackList.
   template<typename TrackKind>
   Track *Add(std::unique_ptr<TrackKind> &&t);
   template<typename TrackKind>
   Track *AddToHead(std::unique_ptr<TrackKind> &&t);

#ifdef __AUDACITY_OLD_STD__
   template<typename TrackKind>
   Track *Add(std::shared_ptr<TrackKind> &&t);
#endif

   /// Replace first track with second track, give back a holder
   value_type Replace(Track * t, value_type &&with);

   /// Remove this Track or all children of this TrackList.
   /// Return an iterator to what followed the removed track.
   TrackNodePointer Remove(Track *t);

   /// Make the list empty
   void Clear(bool sendEvent = true);

   /** Select a track, and if it is linked to another track, select it, too. */
   void Select(Track * t, bool selected = true);

   /** If this track is linked to another track (the track immediately before or
   * after it), return its partner. Otherwise return null. */
   Track *GetLink(Track * t) const;

   Track *GetPrev(Track * t, bool linked = false) const;

   /** Return a track in the list that comes after Track t
     * @param t a track in the list
     * @param linked if true, skips over linked tracks, if false returns the next track even if it is a linked track
    **/
   Track *GetNext(Track * t, bool linked = false) const;
   int GetGroupHeight(Track * t) const;

   bool CanMoveUp(Track * t) const;
   bool CanMoveDown(Track * t) const;

   bool MoveUp(Track * t);
   bool MoveDown(Track * t);
   bool Move(Track * t, bool up) { return up ? MoveUp(t) : MoveDown(t); }

   TimeTrack *GetTimeTrack();
   const TimeTrack *GetTimeTrack() const;

   /** \brief Find out how many channels this track list mixes to
   *
   * This is used in exports of the tracks to work out whether to export in
   * Mono, Stereo etc. @param selectionOnly Whether to consider the entire track
   * list or only the selected members of it
   */
   unsigned GetNumExportChannels(bool selectionOnly) const;

   WaveTrackArray GetWaveTrackArray(bool selectionOnly, bool includeMuted = true);
   WaveTrackConstArray GetWaveTrackConstArray(bool selectionOnly, bool includeMuted = true) const;

#if defined(USE_MIDI)
   NoteTrackArray GetNoteTrackArray(bool selectionOnly);
#endif

   /// Mainly a test function. Uses a linear search, so could be slow.
   bool Contains(const Track * t) const;

   bool IsEmpty() const;
   int GetCount() const;

   double GetStartTime() const;
   double GetEndTime() const;

   double GetMinOffset() const;
   int GetHeight() const;

#if LEGACY_PROJECT_FILE_SUPPORT
   // File I/O
   bool Load(wxTextFile * in, DirManager * dirManager) override;
   bool Save(wxTextFile * out, bool overwrite) override;
#endif

private:

   bool isNull(TrackNodePointer p) const
   { return p == end(); }
   void setNull(TrackNodePointer &p)
   { p = end(); }
   bool hasPrev(TrackNodePointer p) const
   { return p != begin(); }

   void DoAssign(const TrackList &that);
       
   void RecalcPositions(TrackNodePointer node);
   void UpdatedEvent(TrackNodePointer node);
   void ResizedEvent(TrackNodePointer node);

   void SwapNodes(TrackNodePointer s1, TrackNodePointer s2);
};

class AUDACITY_DLL_API TrackFactory
{
 private:
   TrackFactory(const std::shared_ptr<DirManager> &dirManager, const ZoomInfo *zoomInfo):
      mDirManager(dirManager)
      , mZoomInfo(zoomInfo)
   {
   }

   const std::shared_ptr<DirManager> mDirManager;
   const ZoomInfo *const mZoomInfo;
   friend class AudacityProject;
   friend class BenchmarkDialog;

 public:
   // These methods are defined in WaveTrack.cpp, NoteTrack.cpp,
   // LabelTrack.cpp, and TimeTrack.cpp respectively
   std::unique_ptr<WaveTrack> DuplicateWaveTrack(WaveTrack &orig);
   std::unique_ptr<WaveTrack> NewWaveTrack(sampleFormat format = (sampleFormat)0,
                           double rate = 0);
   std::unique_ptr<LabelTrack> NewLabelTrack();
   std::unique_ptr<TimeTrack> NewTimeTrack();
#if defined(USE_MIDI)
   std::unique_ptr<NoteTrack> NewNoteTrack();
#endif
};

#endif
