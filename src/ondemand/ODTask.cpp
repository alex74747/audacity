/**********************************************************************

  Audacity: A Digital Audio Editor

  ODTask.cpp

  Created by Michael Chinen (mchinen)
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODTask
\brief ODTask is an abstract class that outlines the methods that will be used to
support On-Demand background loading of files.  These ODTasks are generally meant to be run
in a background thread.

*//*******************************************************************/


#include "ODTask.h"

#include "ODManager.h"
#include "../WaveClip.h"
#include "../WaveTrack.h"
#include "../Project.h"
#include "../UndoManager.h"
//temporarily commented out till it is added to all projects
//#include "../Profiler.h"

#include <thread>


wxDEFINE_EVENT(EVT_ODTASK_COMPLETE, wxCommandEvent);

/// Constructs an ODTask
ODTask::ODTask()
{
   static int sTaskNumber=0;
   mTaskNumber = sTaskNumber++;
}

//outside code must ensure this task is not scheduled again.
void ODTask::TerminateAndBlock()
{
   mTerminate.store( true, std::memory_order_relaxed );

   //one mutex pair for the exit of the function
   mBlockUntilTerminateMutex.Lock();
//TODO lock mTerminate?
   mBlockUntilTerminateMutex.Unlock();

   //wait till we are out of doSome() to terminate.
   Terminate();
}

///Do a modular part of the task.  For example, if the task is to load the entire file, load one BlockFile.
///Relies on DoSomeInternal(), which the subclasses must implement.
///@param amountWork the percent amount of the total job to do.  1.0 represents the entire job.  the default of 0.0
/// will do the smallest unit of work possible
void ODTask::DoSome(float amountWork)
{
   SetIsRunning(true);
   auto cleanup = finally( [&]{ SetIsRunning(false); } );

   mBlockUntilTerminateMutex.Lock();

//   wxPrintf("%s %i subtask starting on NEW thread with priority\n", GetTaskName(),GetTaskNumber());

   float workUntil = amountWork+PercentComplete();

   //check periodically to see if we should exit.
   auto terminate = [this]{
      return mTerminate.load( std::memory_order_relaxed ); };

   if( terminate() ) {
      mBlockUntilTerminateMutex.Unlock();
      return;
   }

   Update();

   if(UsesCustomWorkUntilPercentage())
      workUntil = ComputeNextWorkUntilPercentageComplete();

   if(workUntil<PercentComplete())
      workUntil = PercentComplete();

   //Do Some of the task.

   while(PercentComplete() < workUntil && PercentComplete() < 1.0 && !terminate())
   {
      std::this_thread::yield();
      //release within the loop so we can cut the number of iterations short

      DoSomeInternal();
      //check to see if ondemand has been called
      if(GetNeedsODUpdate() && PercentComplete() < 1.0)
         ODUpdate();
   }

   //if it is not done, put it back onto the ODManager queue.
   if(PercentComplete() < 1.0&& !terminate())
   {
      ODManager::Instance()->AddTask(this);

      //we did a bit of progress - we should allow a resave.
      ODLocker locker{ &AllProjects::Mutex() };
      for ( auto pProject : AllProjects{} )
      {
         if(IsTaskAssociatedWithProject(pProject.get()))
         {
            //mark the changes so that the project can be resaved.
            UndoManager::Get( *pProject ).SetODChangesFlag();
            break;
         }
      }


//      wxPrintf("%s %i is %f done\n", GetTaskName(),GetTaskNumber(),PercentComplete());
   }
   else
   {
      //for profiling, uncomment and look in audacity.app/exe's folder for AudacityProfile.txt
      //static int tempLog =0;
      //if(++tempLog % 5==0)
         //END_TASK_PROFILING("On Demand Drag and Drop 5 80 mb files into audacity, 5 wavs per task");
      //END_TASK_PROFILING("On Demand open an 80 mb wav stereo file");

      wxCommandEvent event( EVT_ODTASK_COMPLETE );

      ODLocker locker{ &AllProjects::Mutex() };
      for ( auto pProject : AllProjects{} )
      {
         if(IsTaskAssociatedWithProject(pProject.get()))
         {
            //this assumes tasks are only associated with one project.
            pProject->wxEvtHandler::AddPendingEvent(event);
            //mark the changes so that the project can be resaved.
            UndoManager::Get( *pProject ).SetODChangesFlag();
            break;
         }
      }

//      wxPrintf("%s %i complete\n", GetTaskName(),GetTaskNumber());
   }
   mBlockUntilTerminateMutex.Unlock();
}

bool ODTask::IsTaskAssociatedWithProject(AudacityProject* proj)
{
   for (auto tr : TrackList::Get( *proj ).Any<const WaveTrack>())
   {
      //go over all tracks in the project
      //look inside our task's track list for one that matches this projects one.
      mWaveTrackMutex.Lock();
      for(int i=0;i<(int)mWaveTracks.size();i++)
      {
         if ( mWaveTracks[i].lock().get() == tr )
         {
            //if we find one, then the project is associated with us;return true
            mWaveTrackMutex.Unlock();
            return true;
         }
      }
      mWaveTrackMutex.Unlock();
   }

   return false;

}

void ODTask::ODUpdate()
{
   Update();
   ResetNeedsODUpdate();
}

void ODTask::SetIsRunning(bool value)
{
   mIsRunning.store( value, std::memory_order_release );
}

bool ODTask::IsRunning()
{
   return mIsRunning.load( std::memory_order_acquire );
}

sampleCount ODTask::GetDemandSample() const
{
   return mDemandSample.load( std::memory_order_acquire );
}

void ODTask::SetDemandSample(sampleCount sample)
{
   mDemandSample.store( sample, std::memory_order_release );
}


///return the amount of the task that has been completed.  0.0 to 1.0
float ODTask::PercentComplete()
{
   return mPercentComplete.load( std::memory_order_acquire );
}

void ODTask::SetPercentComplete( float complete )
{
   mPercentComplete.store( complete, std::memory_order_release );
}

///return
bool ODTask::IsComplete()
{
   return PercentComplete() >= 1.0 && !IsRunning();
}


std::shared_ptr< WaveTrack > ODTask::GetWaveTrack(int i)
{
   std::shared_ptr< WaveTrack > track;
   mWaveTrackMutex.Lock();
   if(i<(int)mWaveTracks.size())
      track = mWaveTracks[i].lock();
   mWaveTrackMutex.Unlock();
   return track;
}

///Sets the wavetrack that will be analyzed for ODPCMAliasBlockFiles that will
///have their summaries computed and written to disk.
void ODTask::AddWaveTrack( const std::shared_ptr< WaveTrack > &track)
{
   mWaveTracks.push_back(track);
}


int ODTask::GetNumWaveTracks()
{
   int num;
   mWaveTrackMutex.Lock();
   num = (int)mWaveTracks.size();
   mWaveTrackMutex.Unlock();
   return num;
}


void ODTask::SetNeedsODUpdate()
{
   mNeedsODUpdate.store( true, std::memory_order_release );
}
bool ODTask::GetNeedsODUpdate()
{
   return mNeedsODUpdate.load( std::memory_order_acquire );
}

void ODTask::ResetNeedsODUpdate()
{
   mNeedsODUpdate.store( false, std::memory_order_relaxed );
}

///does an od update and then recalculates the data.
void ODTask::RecalculatePercentComplete()
{
   if(GetNeedsODUpdate())
   {
      ODUpdate();
      CalculatePercentComplete();
   }
}

///changes the tasks associated with this Waveform to process the task from a different point in the track
///@param track the track to update
///@param seconds the point in the track from which the tasks associated with track should begin processing from.
void ODTask::DemandTrackUpdate(WaveTrack* track, double seconds)
{
   bool demandSampleChanged=false;
   mWaveTrackMutex.Lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if ( track == mWaveTracks[i].lock().get() )
      {
         auto newDemandSample = (sampleCount)(seconds * track->GetRate());
         demandSampleChanged = newDemandSample != GetDemandSample();
         SetDemandSample(newDemandSample);
         break;
      }
   }
   mWaveTrackMutex.Unlock();

   if(demandSampleChanged)
      SetNeedsODUpdate();

}


void ODTask::StopUsingWaveTrack(WaveTrack* track)
{
   mWaveTrackMutex.Lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if(mWaveTracks[i].lock().get() == track)
         mWaveTracks[i].reset();
   }
   mWaveTrackMutex.Unlock();
}

///Replaces all instances to a wavetrack with a NEW one, effectively transferring the task.
void ODTask::ReplaceWaveTrack(Track *oldTrack,
   const std::shared_ptr< Track > &newTrack)
{
   mWaveTrackMutex.Lock();
   for(size_t i=0;i<mWaveTracks.size();i++)
   {
      if(oldTrack == mWaveTracks[i].lock().get())
      {
         mWaveTracks[i] = std::static_pointer_cast<WaveTrack>( newTrack );
      }
   }
   mWaveTrackMutex.Unlock();
}
