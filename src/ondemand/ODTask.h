/**********************************************************************

  Audacity: A Digital Audio Editor

  ODTask.h

  Created by Michael Chinen (mchinen)
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODTask
\brief ODTask is an abstract class that outlines the methods that will be used to
support On-Demand background loading of files.  These ODTasks are generally meant to be run
in a background thread.

*//*******************************************************************/




#ifndef __AUDACITY_ODTASK__
#define __AUDACITY_ODTASK__

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <wx/event.h> // to declare custom event type

#include "audacity/Types.h"

class AudacityProject;
class Track;
class WaveTrack;


wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_ODTASK_COMPLETE, wxCommandEvent);

/// A class representing a modular task to be used with the On-Demand structures.
class ODTask /* not final */
{
 public:
   enum {
      eODNone     =  0x00000000,
      eODFLAC     =  0x00000001,
      eODMP3      =  0x00000002,
      eODFFMPEG   =  0x00000004,
      eODPCMSummary  = 0x00001000,
      eODOTHER    =  0x10000000,
   } ODTypeEnum;
   // Constructor / Destructor

   /// Constructs an ODTask
   ODTask();

   virtual ~ODTask();

 protected:
   // The callbacks to these overridables are guaranteed to be serialized with
   // each other, and with the addition of the task to the task manager, and
   // the destruction of the task

   ///calculates the fraction complete from existing data.
   virtual float ComputeFractionComplete() = 0;

   ///pure virtual function that does some part of the task this object represents.
   ///this function is meant to be called repeatedly until mPercentComplete reaches 1.0.
   ///Does the smallest unit of work for this task.
   virtual void DoSomeInternal() = 0;

   ///virtual method called before DoSomeInternal is used from DoSome.
   /// Allows the task to check for messages from its environment.
   virtual void Update(){}

 public:
   //clones everything except information about the tracks.
   virtual std::unique_ptr<ODTask> Clone() const = 0;

   void SetThread( std::thread &&thread ) { mThread = std::move( thread ); }

   ///Subclasses should override to return respective type.
   virtual unsigned int GetODType(){return eODNone;}


///Do a modular part of the task.  For example, if the task is to load the entire file, load one BlockFile.
///Relies on DoSomeInternal(), which is the subclasses must implement.
///@param amountWork between 0 and 1, the fraction of the total job to do.
/// will do at least the smallest unit of work possible
///@return true if the task should be scheduled to run again
   bool DoSome(float amountWork=0.0);

   ///Call DoSome until FractionComplete >= 1.0
   void DoAll();

   float FractionComplete();
   void SetFractionComplete( float complete );

   virtual bool UsesCustomNextFraction(){return false;}
   virtual float ComputeNextFractionComplete(){return 1.0;}

   ///returns whether or not this task and another task can merge together, as when we make two mono tracks stereo.
   ///for Loading/Summarizing, this is not an issue because the entire track is processed
   ///Effects that affect portions of a track will need to check this.
   bool CanMergeWith(ODTask* otherTask){return strcmp(GetTaskName(),otherTask->GetTaskName())==0;}

   virtual void StopUsingWaveTrack(WaveTrack* track);

   ///Replaces all instances to a wavetrack with a NEW one, effectively transferring the task.
   ///ODTask has no wavetrack, so it does nothing.  But subclasses that do should override this.
   virtual void ReplaceWaveTrack(Track *oldTrack,
      const std::shared_ptr< Track > &newTrack);

    ///Adds a WaveTrack to do the task for
   void AddWaveTrack( const std::shared_ptr< WaveTrack > &track);
   virtual int GetNumWaveTracks();
   virtual std::shared_ptr< WaveTrack > GetWaveTrack(int i);

   ///changes the tasks associated with this Waveform to process the task from a different point in the track
   virtual void DemandTrackUpdate(WaveTrack* track, double seconds);

   void Join();

   virtual const char* GetTaskName(){return "ODTask";}

   virtual sampleCount GetDemandSample() const;

   virtual void SetDemandSample(sampleCount sample);

   ///does an od update and then recalculates the data.
   void ReUpdateFractionComplete();

   ///returns the number of tasks created before this instance.
   int GetTaskNumber(){return mTaskNumber;}

   void SetNeedsODUpdate();

   virtual TranslatableString GetTip()=0;

    ///returns true if the task is associated with the project.
   virtual bool IsTaskAssociatedWithProject(AudacityProject* proj);

 private:
   ///method called in DoSome to check whether the user has demanded some OD
   //function so that the ODTask can readjust its computation order.
   ///Calls Update() if demanded.
   ///Returns true if work is complete; else, there may have been a concurrent
   ///demand for more work during the update.
   bool ODUpdate();

 protected:
   int   mTaskNumber;
   std::atomic<float> mFractionComplete{ 0.0f };
   std::atomic< bool > mTerminate{ false };

   std::vector< std::weak_ptr< WaveTrack > > mWaveTracks;
   std::mutex     mWaveTrackMutex;

   mutable std::atomic<sampleCount> mDemandSample{ 0 };

   private:

   std::atomic< unsigned > mNeedsODUpdate{ 0u };
   std::thread mThread;
};

#endif

