/**********************************************************************

  Audacity: A Digital Audio Editor

  ODManager.h

  Created by Michael Chinen (mchinen) on 6/8/08
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODManager
\brief A singleton that manages currently running Tasks on an arbitrary
number of threads.

*//*******************************************************************/

#ifndef __AUDACITY_ODMANAGER__
#define __AUDACITY_ODMANAGER__

#include <atomic>
#include <condition_variable>
#include <thread>
#include <vector>
#include <wx/event.h> // for DECLARE_EXPORTED_EVENT_TYPE

// This event is posted to the application
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_ODTASK_UPDATE, wxCommandEvent);

///wxstring compare function for sorting case, which is needed to load correctly.
int CompareNoCaseFileName(const wxString& first, const wxString& second);
/// A singleton that manages currently running Tasks on an arbitrary
/// number of threads.
class TranslatableString;
class Track;
class WaveTrack;
class ODTask;
class ODWaveTrackTaskQueue;
class ODManager final
{
 public:
   using QueuesLocker = std::lock_guard< std::mutex >;

   ///Get the singleton instance (creating it only on demand)
   static ODManager* Instance();

   ///Kills the ODMananger Thread.
   static void Quit();

   ///changes the tasks associated with this Waveform to process the task from a different point in the track
   void DemandTrackUpdate(Track* track, double seconds);

   ///Reduces the count of current threads running.  Meant to be called when ODTaskThreads end in their own threads.  Thread-safe.
   void DecrementCurrentThreads();

   ///Adds a wavetrack, creates a queue member.
   void AddNewTask( std::unique_ptr<ODTask> mtask );

   ///if it shares a queue/task, creates a NEW queue/task for the track, and removes it from any previously existing tasks.
   void MakeWaveTrackIndependent( const std::shared_ptr< Track > &track);

   ///attach the track in question to another, already existing track's queues and tasks.  Remove the task/tracks.
   ///Returns success if it was possible..  Some ODTask conditions make it impossible until the Tasks finish.
   ///Callable from the main thread
   bool MakeWaveTrackDependent(
      const std::shared_ptr< Track > &dependentTrack,
      Track* masterTrack
   );

   ///if oldTrack is being watched,
   ///replace the wavetrack whose wavecache the gui watches for updates
   void ReplaceWaveTrack(Track *oldTrack,
      const std::shared_ptr< Track > &newTrack);

   ///Adds a task to the running queue.  Thread-safe.
   void AddTask(ODTask* task);

   ///sets a flag that is set if we have loaded some OD blockfiles from PCM.
   static void MarkLoadedODFlag();

   ///resets a flag that is set if we have loaded some OD blockfiles from PCM.
   static void UnmarkLoadedODFlag();

   ///returns a flag that is set if we have loaded some OD blockfiles from PCM.
   static bool HasLoadedODFlag();

   ///returns whether or not the singleton instance was created yet
   static bool IsInstanceCreated();

   ///fills in the status bar message for a given track
   void FillTipForWaveTrack( const Track * t, TranslatableString &tip );

   ///Gets the total percent complete for all tasks combined, and number of
   // tasks
   ///(Answer might be invalidated by action of other threads)
   std::pair< float, int > GetOverallCompletion();

   // RAII object for pausing and resuming.
   class Pauser
   {
      //Pause/unpause all OD Tasks.  Does not occur immediately.
      static void Pause(bool pause = true);
      static void Resume();
   public:
      Pauser() { Pause(); }
      ~Pauser() { Resume(); }
      Pauser(const Pauser&) PROHIBITED;
      Pauser &operator= (const Pauser&) PROHIBITED;
   };



  protected:
   //private constructor - Singleton.
   ODManager();
   //private constructor - DELETE with static method Quit()
   friend std::default_delete < ODManager > ;
   ~ODManager();
   ///Launches a thread for the manager and starts accepting Tasks.
   void Init();

   /// The main loop for the manager.
   /// Runs in its own thread, which spawns other worker threads.
   void DispatchLoop();

   ///Remove references in our array to Tasks that have been completed/Schedule NEW ones
   void UpdateQueues( ODTask *pTask );
   void DeleteQueue( size_t ii );
   void RemoveTaskIfInQueue(ODTask* task);

   //instance
   static std::unique_ptr<ODManager> pMan;

   //List of tracks and their active and inactive tasks.
   std::vector<std::unique_ptr<ODWaveTrackTaskQueue>> mQueues;
   std::mutex mQueuesMutex;

   //List of current Task to do.
   std::vector<ODTask*> mTasks;
   //mutex for above variable
   std::mutex mTasksMutex;

   ///Number of threads currently running.   Accessed thru multiple threads
   std::atomic< unsigned > mCurrentThreads{ 0 };

   ///Maximum number of threads allowed out.
   int mMaxThreads;

   std::atomic< bool > mTerminate{ false };

   //for the queue not empty comdition
   // Must NOT be locked while already holding mTasksMutex!
   std::mutex         mQueueNotEmptyCondLock;
   std::condition_variable mQueueNotEmptyCond;
   std::thread mDispatcher;
};

#endif

