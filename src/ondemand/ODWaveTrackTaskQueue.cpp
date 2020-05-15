/**********************************************************************

  Audacity: A Digital Audio Editor

  ODWaveTrackTaskQueue.h

  Created by Michael Chinen (mchinen) on 6/11/08
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODWaveTrackTaskQueue
\brief watches over all to be done (not yet started and started but not finished)
tasks associated with a WaveTrack.

*//*******************************************************************/

#include "../Audacity.h"
#include "ODWaveTrackTaskQueue.h"

#include "ODTask.h"
#include "../WaveTrack.h"
/// Constructs an ODWaveTrackTaskQueue
ODWaveTrackTaskQueue::ODWaveTrackTaskQueue()
{
}

ODWaveTrackTaskQueue::~ODWaveTrackTaskQueue()
{
}

///returns whether or not this queue's task list and another's can merge together, as when we make two mono tracks stereo.
bool ODWaveTrackTaskQueue::CanMergeWith( const TasksLocker &myLocker,
   const TasksLocker &otherLocker,
   ODWaveTrackTaskQueue* otherQueue)
{
   //have to be very careful when dealing with two lists that need to be locked.
   // Define an ordering to prevent deadlock!

   if( GetNumTasks( myLocker ) != otherQueue->GetNumTasks( otherLocker ) )
      return false;

   for( unsigned int i = 0; i < mTasks.size(); i++ )
      if( !mTasks[i]->CanMergeWith( otherQueue->GetTask( otherLocker, i) ) )
         return false;
   return true;
}

///add track to the masterTrack's queue - this will allow future ODScheduling to affect them together.
/// sets the NeedODUpdateFlag since we don't want the head task to finish without haven't dealt with the dependent
///
///@param track the track to bring into the tasks AND tracklist for this queue
void ODWaveTrackTaskQueue::MergeWaveTrack( const TasksLocker &,
   const std::shared_ptr< WaveTrack > &track)
{
   AddWaveTrack( track );

   for(unsigned int i=0;i<mTasks.size();i++)
   {
      mTasks[i]->AddWaveTrack(track);
      mTasks[i]->SetNeedsODUpdate();
   }
}

///returns true if the argument is in the WaveTrack list.
bool ODWaveTrackTaskQueue::ContainsWaveTrack( const WaveTrack* track )
{
   for(unsigned int i=0;i<mTracks.size();i++)
   {
      if ( mTracks[i].lock().get() == track )
         return true;
   }
   return false;
}

///Adds a track to the associated list.
void ODWaveTrackTaskQueue::AddWaveTrack(
   const std::shared_ptr< WaveTrack > &track)
{
   if(track)
      mTracks.push_back(track);
}

void ODWaveTrackTaskQueue::AddTask( const TasksLocker &,
   std::unique_ptr<ODTask> mtask )
{
   ODTask *task = mtask.get();

   //take all of the tracks in the task.
   for(int i=0;i<task->GetNumWaveTracks();i++)
   {
      //task->GetWaveTrack(i) may return NULL, but we handle it by checking before using.
      //The other worry that the WaveTrack returned and was deleted in the meantime is also
      //handled by keeping standard weak pointers to tracks, which give thread safety.
      mTracks.push_back(task->GetWaveTrack(i));
   }

   mTasks.push_back(std::move(mtask));
}

///changes the tasks associated with this Waveform to process the task from a different point in the track
///@param track the track to update
///@param seconds the point in the track from which the tasks associated with track should begin processing from.
void ODWaveTrackTaskQueue::DemandTrackUpdate( WaveTrack* track, double seconds )
{
   if(track)
   {
      for(unsigned int i=0;i<mTasks.size();i++)
         mTasks[i]->DemandTrackUpdate(track,seconds);
   }
}


//Replaces all instances of a wavetracck with a NEW one (effectively transferes the task.)
void ODWaveTrackTaskQueue::ReplaceWaveTrack(
   Track *oldTrack,
   const std::shared_ptr<Track> &newTrack )
{
   if(oldTrack)
   {
      for(unsigned int i=0;i<mTracks.size();i++)
         if ( mTracks[i].lock().get() == oldTrack )
            mTracks[i] = std::static_pointer_cast<WaveTrack>( newTrack );

      mTasksMutex.Lock();
      for(unsigned int i=0;i<mTasks.size();i++)
         mTasks[i]->ReplaceWaveTrack( oldTrack, newTrack );
      mTasksMutex.Unlock();
   }
}

///returns the number of ODTasks in this queue
size_t ODWaveTrackTaskQueue::GetNumTasks( const TasksLocker & )
{
   return mTasks.size();
}

///returns a ODTask at position x
ODTask* ODWaveTrackTaskQueue::GetTask( const TasksLocker &, size_t x )
{
   if ( x < mTasks.size() )
      return mTasks[x].get();
   return nullptr;
}



//returns true if either tracks or tasks are empty
bool ODWaveTrackTaskQueue::IsEmpty( const TasksLocker & )
{
   Compress();
   return mTracks.empty() || mTasks.empty();
}

///Removes and deletes the front task from the list.
void ODWaveTrackTaskQueue::RemoveFrontTask( const TasksLocker & )
{
   if(mTasks.size())
   {
      //wait for the task to stop running.
      mTasks.erase(mTasks.begin());
   }
}

///gets the front task for immediate execution
ODTask* ODWaveTrackTaskQueue::GetFrontTask( const TasksLocker & )
{
   if( mTasks.size() )
      return mTasks[0].get();
   return nullptr;
}

///fills in the status bar message for a given track
void ODWaveTrackTaskQueue::FillTipForWaveTrack(
   const WaveTrack * t, TranslatableString &tip )
{
   TasksLocker locker{ &mTasksMutex };
   if ( ContainsWaveTrack( t ) && GetNumTasks( locker ) )
   {

    //  if(GetNumTasks()==1)
      mTipMsg = XO("%s %2.0f%% complete. Click to change task focal point.")
         .Format(
            GetFrontTask( locker )->GetTip(),
            GetFrontTask( locker )->FractionComplete()*100.0 );
     // else
       //  msg = XO("%s %d additional tasks remaining.")
       //     .Format( GetFrontTask()->GetTip(), GetNumTasks());

      tip = mTipMsg;

   }
}

// Call this only within the scope of a lock on the set of tracks!
void ODWaveTrackTaskQueue::Compress()
{
   auto begin = mTracks.begin(), end = mTracks.end(),
   new_end = std::remove_if( begin, end,
      []( const std::weak_ptr<WaveTrack> &ptr ){ return ptr.expired(); } );
   mTracks.erase( new_end, end );
}
