/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2010 Audacity Team
   File License: wxWidgets

   Michael Chinen

******************************************************************//**

\file ODManager.cpp
\brief Singleton ODManager class.  Is the bridge between client side
ODTask requests and internals.

*//*******************************************************************/

#include "../Audacity.h"
#include "ODManager.h"

#include "ODTask.h"
#include "ODWaveTrackTaskQueue.h"
#include "../Project.h"
#include <wx/utils.h>
#include <wx/wx.h>
#include <wx/event.h>
#include <thread>

static std::atomic<bool> gManagerCreated{ false };
std::atomic< bool > gPause{ false }; //to be loaded in and used with Pause/Resume before ODMan init.
/// a flag that is set if we have loaded some OD blockfiles from PCM.
static bool sHasLoadedOD=false;

std::unique_ptr<ODManager> ODManager::pMan;

ODManager *ODManager::Instance()
{
   static std::once_flag flag;
   std::call_once( flag, []{
      pMan.reset( safenew ODManager );
      pMan->Init();
      gManagerCreated.store( true, std::memory_order_release );
   } );
   return pMan.get();
}

wxDEFINE_EVENT(EVT_ODTASK_UPDATE, wxCommandEvent);

//using this with wxStringArray::Sort will give you a list that
//is alphabetical, without depending on case.  If you use the
//default sort, you will get strings with 'R' before 'a', because it is in caps.
int CompareNoCaseFileName(const wxString& first, const wxString& second)
{
   return first.CmpNoCase(second);
}

//private constructor - Singleton.
ODManager::ODManager()
{
}

//private destructor - DELETE with static method Quit()
ODManager::~ODManager()
{
   {
      // Must hold mutex while making the condition (termination) true
      std::lock_guard< std::mutex > lock{ mQueueNotEmptyCondLock };
      mTerminate.store( true, std::memory_order_relaxed );
      //signal the queue not empty condition since the ODMan thread will wait on the queue condition
      mQueueNotEmptyCond.notify_one();
   }

   mDispatcher.join();

   // get rid of all the queues.
   // DeleteQueue properly terminates worker threads and destroys task objects.
   while ( auto size = mQueues.size() )
      DeleteQueue( size - 1 );
}

///Adds a task to running queue.  Thread-safe.
void ODManager::AddTask(ODTask* task)
{
   bool hadTask = false;

   // Must hold mutex while making the condition (nonempty queue) true
   std::lock_guard< std::mutex > lock{ mQueueNotEmptyCondLock };
   mTasksMutex.Lock();
   hadTask = !mTasks.empty();
   mTasks.push_back(task);
   mTasksMutex.Unlock();
   //signal the queue not empty condition.
   bool paused = gPause.load( std::memory_order_acquire );

   //don't signal if we are paused since if we wake up the loop it will start processing other tasks while paused
   // Also shouldn't need a signal if tasks were already nonempty
   if ( !(paused || hadTask) )
      mQueueNotEmptyCond.notify_one();
}

///removes a task from the active task queue
void ODManager::RemoveTaskIfInQueue(ODTask* task)
{
   mTasksMutex.Lock();
   //linear search okay for now, (probably only 1-5 tasks exist at a time.)
   for(unsigned int i=0;i<mTasks.size();i++)
   {
      if(mTasks[i]==task)
      {
         mTasks.erase(mTasks.begin()+i);
         break;
      }
   }
   mTasksMutex.Unlock();

}

///Adds a NEW task to the queue.  Creates a queue if the tracks associated with the task is not in the list
///
///@param task the task to add
///@param lockMutex locks the mutexes if true (default).  This function is used within other ODManager calls, which many need to set this to false.
void ODManager::AddNewTask(std::unique_ptr<ODTask> &&mtask, bool lockMutex)
{
   auto task = mtask.get();
   ODWaveTrackTaskQueue* queue = NULL;

   if(lockMutex)
      mQueuesMutex.Lock();
   for ( const auto &pQueue : mQueues )
   {
      //search for a task containing the lead track.  wavetrack removal is threadsafe and bound to the mQueuesMutex
      //note that GetWaveTrack is not threadsafe, but we are assuming task is not running on a different thread yet.
      ODWaveTrackTaskQueue::TracksLocker locker{ &pQueue->mTracksMutex };
      if( pQueue->ContainsWaveTrack( locker, task->GetWaveTrack(0).get() ) )
      {
         //Add it to the existing queue but keep the lock since this reference can be deleted.
         queue->AddTask( locker, std::move(mtask) );
         if(lockMutex)
            mQueuesMutex.Unlock();
         queue = pQueue.get();
         break;
      }
   }

   if( !queue )
   {
      //Make a NEW one, add it to the local track queue, and to the immediate running task list,
      //since this task is definitely at the head
      auto newqueue = std::make_unique<ODWaveTrackTaskQueue>();
      {
         ODWaveTrackTaskQueue::TracksLocker locker{ &newqueue->mTracksMutex };
         newqueue->AddTask( locker, std::move(mtask) );
      }
      mQueues.push_back(std::move(newqueue));
      if(lockMutex)
         mQueuesMutex.Unlock();

      AddTask(task);
   }
}

bool ODManager::IsInstanceCreated()
{
   return gManagerCreated.load( std::memory_order_acquire );
}

///Launches a thread for the manager and starts accepting Tasks.
void ODManager::Init()
{
   mCurrentThreads = 0;
   mMaxThreads = 5;

   //   wxLogDebug(wxT("Initializing ODManager...Creating manager thread"));

//   startThread->SetPriority(0);//default of 50.
//   wxPrintf("starting thread from init\n");

   mDispatcher = std::thread{ [this]{ DispatchLoop(); } };

//   wxPrintf("started thread from init\n");
   //destruction of thread is taken care of by thread library
}

void ODManager::DecrementCurrentThreads()
{
   mCurrentThreads.fetch_sub( 1, std::memory_order_release );
}

///Main loop for managing threads and tasks.
/// Runs in its own thread, which spawns other worker threads.
void ODManager::DispatchLoop()
{
   int  numQueues=0;

   int mNeedsDraw=0;

   auto tasksInArray = [this](const ODLocker&){
      return !mTasks.empty();
   };

   auto paused = []{ return gPause.load( std::memory_order_acquire ); };

   //wxLog calls not threadsafe.  are printfs?  thread-messy for sure, but safe?
//   wxPrintf("ODManager thread strating \n");
   //TODO: Figure out why this has no effect at all.
   //wxThread::This()->SetPriority(30);
   while( true )
   {
//    wxPrintf("ODManager thread running \n");

      //start some threads if necessary

      //use a condition variable to block here instead of a sleep.

      // JKC: If there are no tasks ready to run, or we're paused then
      // we wait for there to be tasks in the queue.
      // PRL but we also reply promptly to the "poison pill" sent from main
      // thread when ODManager is being destroyed.
      {
         bool terminated = false;
         std::unique_lock< std::mutex > locker{ mQueueNotEmptyCondLock };
         mQueueNotEmptyCond.wait( locker, [&]{
            if ( (terminated = mTerminate.load( std::memory_order_relaxed )) )
               return true;
            if ( paused() )
               return false;
            return tasksInArray( ODLocker { &mTasksMutex } );
         } );
         if ( terminated )
            return;
      }

      // keep adding tasks if there is work to do, up to the limit.
      while(!paused() &&
         (mCurrentThreads.load( std::memory_order_acquire ) < mMaxThreads))
      {
         // Retest for tasks at the top of the loop, because we let go of
         // mTasksMutex, and other functions may destroy tasks.
         ODLocker locker{ &mTasksMutex };
         if ( !tasksInArray( locker ) )
            break;
         mCurrentThreads.fetch_add( 1, std::memory_order_relaxed );

         // recruit a NEW thread.
         auto pTask = mTasks[0];
         pTask->SetThread( std::thread{ [ this, pTask ]{
            //TODO: Figure out why this has no effect at all.
            //wxThread::This()->SetPriority( 40);
            //Do at least 5 percent of the task
            if ( pTask->DoSome(0.05f) )
               AddTask( pTask );
            
            //we should look at our WaveTrack queues to see if we can schedule
            //a NEW task to the running queue.
            UpdateQueues( pTask );

            //release the thread count so that the dispatcher thread knows how
            //many active threads are alive.
            DecrementCurrentThreads();
         } } );

         //thread->SetPriority(10);//default is 50.

         mTasks.erase(mTasks.begin());
      }

      //if there is some ODTask running, then there will be something in the queue.  If so then redraw to show progress
      mQueuesMutex.Lock();
      mNeedsDraw += mQueues.size()>0?1:0;
      numQueues=mQueues.size();
      mQueuesMutex.Unlock();

      //redraw the current project only (ODTasks will send a redraw on complete even if the projects are in the background)
      //we don't want to redraw at a faster rate when we have more queues because
      //this means the CPU is already taxed.  This if statement normalizes the rate
      if((mNeedsDraw>numQueues) && numQueues)
      {
         mNeedsDraw=0;
         wxCommandEvent event( EVT_ODTASK_UPDATE );
         wxTheApp->AddPendingEvent(event);
      }
   }

   //wxLogDebug Not thread safe.
   //wxPrintf("ODManager thread terminating\n");
}

//static function that prevents ODTasks from being scheduled
//does not stop currently running tasks from completing their immediate subtask,
//but presumably they will finish within a second
void ODManager::Pauser::Pause(bool pause)
{
   if(IsInstanceCreated())
   {
      // Must hold mutex while making the condition (unpaused) true
      std::lock_guard< std::mutex > lock{ pMan->mQueueNotEmptyCondLock };
      gPause.store( pause, std::memory_order_release );
      if(!pause)
         //we should check the queue again.
         pMan->mQueueNotEmptyCond.notify_one();
   }
   else
      gPause.store( pause, std::memory_order_relaxed );
}

void ODManager::Pauser::Resume()
{
   Pause(false);
}

void ODManager::Quit()
{
   if(IsInstanceCreated())
   {
      pMan.reset();
   }
}

///replace the wavetrack whose wavecache the gui watches for updates
void ODManager::ReplaceWaveTrack(Track *oldTrack,
   const std::shared_ptr<Track> &newTrack)
{
   mQueuesMutex.Lock();
   for ( const auto &pQueue : mQueues )
   {
      ODWaveTrackTaskQueue::TracksLocker locker{ &pQueue->mTracksMutex };
      pQueue->ReplaceWaveTrack( locker, oldTrack, newTrack );
   }
   mQueuesMutex.Unlock();
}

///if it shares a queue/task, creates a NEW queue/task for the track, and removes it from any previously existing tasks.
void ODManager::MakeWaveTrackIndependent(
   const std::shared_ptr< WaveTrack > &track)
{
   mQueuesMutex.Lock();
   for ( const auto &pQueue : mQueues )
   {
      ODWaveTrackTaskQueue::TracksLocker locker1{ &pQueue->mTracksMutex };
      if( pQueue->ContainsWaveTrack( locker1, track.get() ) )
      {
         const auto owner = pQueue.get();

         //if the wavetrack is in this queue, and is not the only wavetrack,
         //clone the tasks and schedule it.
         // First remove expired weak pointers
         owner->Compress( locker1 );
      
         if ( owner->mTracks.size() < 2 )
            //if there is only one track, it is already independent.
            ;
         else {
            for ( auto &pTrack : owner->mTracks )
            {
               if ( pTrack.lock() == track )
               {
                  pTrack.reset();

                  //clone the items in order and add them to the ODManager.
                  owner->mTasksMutex.Lock();
                  for ( const auto &pTask : owner->mTasks )
                  {
                     auto task = pTask->Clone();
                     pTask->StopUsingWaveTrack( track.get() );
                     //AddNewTask requires us to relinquish this lock. However, it is safe because ODManager::MakeWaveTrackIndependent
                     //has already locked the m_queuesMutex.
                     owner->mTasksMutex.Unlock();
                     //AddNewTask locks the m_queuesMutex which is already locked by ODManager::MakeWaveTrackIndependent,
                     //so we pass a boolean flag telling it not to lock again.
                     this->AddNewTask(std::move(task), false);
                     owner->mTasksMutex.Lock();
                  }
                  owner->mTasksMutex.Unlock();
                  break;
               }
            }
         }
         break;
      }
   }

   mQueuesMutex.Unlock();
}

///attach the track in question to another, already existing track's queues and tasks.  Remove the task/tracks.
///only works if both tracks exist.  Sets needODUpdate flag for the task.  This is complicated and will probably need
///better design in the future.
///@return returns success.  Some ODTask conditions require that the tasks finish before merging.
///e.g. they have different effects being processed at the same time.
bool ODManager::MakeWaveTrackDependent(
   const std::shared_ptr< WaveTrack > &dependentTrack,
   WaveTrack* masterTrack
)
{
   //First, check to see if the task lists are mergeable.  If so, we can simply add this track to the other task and queue,
   //then DELETE this one.
   ODWaveTrackTaskQueue* masterQueue=NULL;
   ODWaveTrackTaskQueue* dependentQueue=NULL;
   unsigned int dependentIndex = 0;
   bool canMerge = false;

   mQueuesMutex.Lock();
   size_t ii = 0;
   for( const auto &pQueue : mQueues )
   {
      ODWaveTrackTaskQueue::TracksLocker locker{ &pQueue->mTracksMutex };
      if ( pQueue->ContainsWaveTrack( locker, masterTrack) )
      {
         masterQueue = pQueue.get();
      }
      else if( pQueue->ContainsWaveTrack( locker, dependentTrack.get() ) )
      {
         dependentQueue = pQueue.get();
         dependentIndex = ii;
      }
      ++ii;
   }
   if(masterQueue&&dependentQueue)
      canMerge=masterQueue->CanMergeWith(dependentQueue);

   //otherwise we need to let dependentTrack's queue live on.  We'll have to wait till the conflicting tasks are done.
   if(!canMerge)
   {
      mQueuesMutex.Unlock();
      return false;
   }
   //then we add dependentTrack to the masterTrack's queue - this will allow future ODScheduling to affect them together.
   //this sets the NeedODUpdateFlag since we don't want the head task to finish without haven't dealt with the dependent
   {
      ODWaveTrackTaskQueue::TracksLocker locker{ &masterQueue->mTracksMutex };
      masterQueue->MergeWaveTrack( locker, dependentTrack );
   }

   //finally remove the dependent track
   DeleteQueue( dependentIndex );
   mQueuesMutex.Unlock();
   return true;
}


///changes the tasks associated with this Waveform to process the task from a different point in the track
///@param track the track to update
///@param seconds the point in the track from which the tasks associated with track should begin processing from.
void ODManager::DemandTrackUpdate(WaveTrack* track, double seconds)
{
   mQueuesMutex.Lock();
   for ( const auto &pQueue : mQueues )
   {
      ODWaveTrackTaskQueue::TracksLocker locker{ &pQueue->mTracksMutex };
      pQueue->DemandTrackUpdate( locker, track, seconds );
   }
   mQueuesMutex.Unlock();
}

///remove tasks from ODWaveTrackTaskQueues that have been done.  Schedules NEW ones if they exist
///Also remove queues that have become empty.
void ODManager::UpdateQueues( ODTask *pTask )
{
   mQueuesMutex.Lock();
   for ( size_t ii = 0; ii < mQueues.size(); ++ii )
   {
      const auto &pQueue = mQueues[ii];
      if ( pTask == pQueue->GetFrontTask() &&
         //there is a chance the task got updated and now has more to do,
         //(like when it is joined with a NEW track)
         //check.
          (pTask->ReUpdateFractionComplete(),
           pTask->FractionComplete() >= 1.0 ) )
      {
         //this should DELETE and remove the front task instance.
         pQueue->RemoveFrontTask();
   
         //schedule next.
         ODWaveTrackTaskQueue::TracksLocker locker{ &pQueue->mTracksMutex };
         if ( !pQueue->IsEmpty( locker ) )
         {
            //we need to release the lock on the queue vector before using the task vector's lock or we deadlock
            //so get a temp.
            ODWaveTrackTaskQueue* queue = pQueue.get();

            AddTask(queue->GetFrontTask());
         }
      }

      //if the queue is empty DELETE it.
      ODWaveTrackTaskQueue::TracksLocker locker{ &pQueue->mTracksMutex };
      if ( pQueue->IsEmpty( locker ) )
         DeleteQueue( ii-- );
   }
   mQueuesMutex.Unlock();
}

void ODManager::DeleteQueue( size_t ii )
{
   const auto ppQueue = mQueues.begin() + ii;
   const auto &pQueue = *ppQueue;

   //we need to DELETE all ODTasks.  We will have to block or wait until block for the active ones.
   while ( pQueue->GetNumTasks() > 0 )
   {
      const auto pTask = pQueue->GetTask(0);
      pTask->Join();//blocks if active.
      //small chance we may have re-added the task back into the queue from a diff thread.  - so remove it if we have.
      RemoveTaskIfInQueue( pTask );
      pQueue->RemoveFrontTask();
   }
   mQueues.erase( ppQueue );
}

//static
///sets a flag that is set if we have loaded some OD blockfiles from PCM.
void ODManager::MarkLoadedODFlag()
{
   sHasLoadedOD = true;
}

//static
///resets a flag that is set if we have loaded some OD blockfiles from PCM.
void ODManager::UnmarkLoadedODFlag()
{
   sHasLoadedOD = false;
}

//static
///returns a flag that is set if we have loaded some OD blockfiles from PCM.
bool ODManager::HasLoadedODFlag()
{
   return sHasLoadedOD;
}

///fills in the status bar message for a given track
void ODManager::FillTipForWaveTrack( const WaveTrack * t, TranslatableString &tip )
{
   mQueuesMutex.Lock();
   for ( const auto &pQueue : mQueues )
   {
      ODWaveTrackTaskQueue::TracksLocker locker{ &pQueue->mTracksMutex };
      pQueue->FillTipForWaveTrack( locker, t, tip );
   }
   mQueuesMutex.Unlock();
}

///Gets the total fraction complete for all tasks combined, weighting the tasks
/// equally.
float ODManager::GetOverallCompletion()
{
   float total=0.0;
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      total+=mQueues[i]->GetFrontTask()->FractionComplete();
   }
   mQueuesMutex.Unlock();

   //avoid div by zero and be thread smart.
   int totalTasks = GetTotalNumTasks();
   return (float) total/(totalTasks>0?totalTasks:1);
}

///Get Total Number of Tasks.
int ODManager::GetTotalNumTasks()
{
   int ret=0;
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      ret+=mQueues[i]->GetNumTasks();
   }
   mQueuesMutex.Unlock();
   return ret;
}
