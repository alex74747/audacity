/**********************************************************************

Audacity: A Digital Audio Editor

@file SyncLock.cpp
@brief Defines groupings of tracks that should keep contents synchronized

Paul Licameli split from Track.cpp

**********************************************************************/
#ifndef __AUDACITY_SYNC_LOCK__
#define __AUDACITY_SYNC_LOCK__

#include "Track.h" // for TrackIterRange
#include "AttachedVirtualFunction.h"

//! Event emitted by the project when sync lock state changes
struct SyncLockChangeEvent : wxEvent{
   explicit SyncLockChangeEvent(bool on);
   ~SyncLockChangeEvent() override;
   wxEvent *Clone() const override;
   bool mIsOn; //!< state sync lock has after the change
};

// Sent to the project when certain settings change
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
   EVT_SYNC_LOCK_CHANGE, SyncLockChangeEvent);

class AUDACITY_DLL_API SyncLockState final
   : public AttachedProjectObject
{
public:
   static SyncLockState &Get( AudacityProject &project );
   static const SyncLockState &Get( const AudacityProject &project );
   explicit SyncLockState( AudacityProject &project );
   SyncLockState( const SyncLockState & ) = delete;
   SyncLockState &operator=( const SyncLockState & ) = delete;

   bool IsSyncLocked() const;
   void SetSyncLock(bool flag);

private:
   AudacityProject &mProject;
   bool mIsSyncLocked{ false };
};

class AUDACITY_DLL_API SyncLock {
public:
   //! @return pTrack is not null, sync lock is on, and some member of its group is selected
   static bool IsSyncLockSelected( const Track *pTrack );

   //! @return pTrack is not null, and is selected, or is sync-lock selected
   static bool IsSelectedOrSyncLockSelected( const Track *pTrack );

   /*! @pre `pTrack->GetOwner() != nullptr` */
   static TrackIterRange< Track > Group( Track *pTrack );

   /*! @copydoc Group */
   static TrackIterRange< const Track > Group( const Track *pTrack )
   {
      return Group(const_cast<Track*>(pTrack)).Filter<const Track>();
   }
};

//! Describes how a track participates in sync-lock groupings
enum class SyncLockPolicy {
   Isolated, //!< Never part of a group
   Grouped, //!< Can be part of a group
   EndSeparator, //!< Delimits the end of a group (of which it is a part)
};

struct GetSyncLockPolicyTag;

//! Describe how this track participates in sync-lock groupings; defaults to Isolated
using GetSyncLockPolicy =
AttachedVirtualFunction<
   GetSyncLockPolicyTag,
   SyncLockPolicy,
   const Track
>;
DECLARE_EXPORTED_ATTACHED_VIRTUAL(AUDACITY_DLL_API, GetSyncLockPolicy);

#endif
