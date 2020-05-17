/**********************************************************************

  Audacity: A Digital Audio Editor

  ODComputeSummaryTask.cpp

  Created by Michael Chinen (mchinen) on 6/8/08.
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODComputeSummaryTask
\brief Computes the summary data for a PCM (WAV) file and writes it to disk,
updating the ODPCMAliasBlockFile and the GUI of the newly available data.

*//*******************************************************************/



#include "ODComputeSummaryTask.h"
#include "../blockfile/ODPCMAliasBlockFile.h"
#include "../Sequence.h"
#include "../WaveClip.h"
#include "../WaveTrack.h"
#include <wx/wx.h>

#include <thread>

//36 blockfiles > 3 minutes stereo 44.1kHz per ODTask::DoSome
#define nBlockFilesPerDoSome 36

///Creates a NEW task that computes summaries for a wavetrack that needs to be specified through SetWaveTrack()
ODComputeSummaryTask::ODComputeSummaryTask()
{
   mMaxBlockFiles = 0;
}

std::unique_ptr<ODTask> ODComputeSummaryTask::Clone() const
{
   auto clone = std::make_unique<ODComputeSummaryTask>();
   clone->mDemandSample = GetDemandSample();
   // This std::move is needed to "upcast" the pointer type
   return std::move(clone);
}


///Computes and writes the data for one BlockFile if it still has a refcount.
void ODComputeSummaryTask::DoSomeInternal()
{
   if ( mBlockFiles.empty() )
      return;

   for(size_t j=0; j < mWaveTracks.size() && mBlockFiles.size();j++)
   {
      bool success = false;
      const auto bf = mBlockFiles[0].lock();

      sampleCount blockStartSample = 0;
      sampleCount blockEndSample = 0;

      if(bf)
      {
         // WriteSummary might throw, but this is a worker thread, so stop
         // the exceptions here!
         success = GuardedCall<bool>( [&] {
            bf->DoWriteSummary();
            return true;
         } );
         blockStartSample = bf->GetStart();
         blockEndSample = blockStartSample + bf->GetLength();
      }
      else
      {
         success = true;
         // The block file disappeared.
         //the waveform in the wavetrack now is shorter, so we need to update mMaxBlockFiles
         //because now there is less work to do.
         mMaxBlockFiles--;
      }

      if (success)
      {
         //take it out of the array - we are done with it.
         mBlockFiles.erase(mBlockFiles.begin());
      }
      else
         // The task does not make progress
         ;

      //update the gui for all associated blocks.  It doesn't matter that we're hitting more wavetracks then we should
      //because this loop runs a number of times equal to the number of tracks, they probably are getting processed in
      //the next iteration at the same sample window.
      if (success && bf) {
         std::lock_guard< std::mutex > locker{ mWaveTrackMutex };
         for(size_t i=0;i<mWaveTracks.size();i++)
         {
            auto waveTrack = mWaveTracks[i].lock();
            if(success && waveTrack)
               static_cast<WaveTrack*>(waveTrack.get())
                  ->AddInvalidRegion(blockStartSample,blockEndSample);
         }
      }
   }
}

///compute the next time we should take a break in terms of overall completion.
///We want to do a constant number of blockfiles.
float ODComputeSummaryTask::ComputeNextFractionComplete()
{
   if(mMaxBlockFiles==0)
     return 1.0;

   return FractionComplete() + ((float)nBlockFilesPerDoSome/(mMaxBlockFiles+1));
}

float ODComputeSummaryTask::ComputeFractionComplete()
{
   if ( mBlockFiles.empty() )
      return 1;
   else if ( mHasUpdateRun )
      return (float) 1.0 - ((float)mBlockFiles.size() / (mMaxBlockFiles+1));
   else
      return 0;
}

///creates the order of the wavetrack to load.
///by default left to right, or frome the point the user has clicked.
void ODComputeSummaryTask::Update()
{
   std::vector< std::weak_ptr< ODPCMAliasBlockFile > > tempBlocks;

{
   std::lock_guard< std::mutex > locker{ mWaveTrackMutex };

   for (const auto &pTrack : mWaveTracks)
   {
      auto waveTrack = pTrack.lock();
      if (waveTrack)
      {
         //gather all the blockfiles that we should process in the wavetrack.
         for (const auto &clip : static_cast<WaveTrack*>(waveTrack.get())
            ->GetAllClips()) {
            auto seq = clip->GetSequence();
            //This lock may be way too big since the whole file is one sequence.
            //TODO: test for large files and find a way to break it down.
            Sequence::DeleteUpdateMutexLocker locker(*seq);

            auto blocks = clip->GetSequenceBlockArray();

            int insertCursor = 0;//OD TODO:see if this works, removed from inner loop (bfore was n*n)

            for (auto &block : *blocks)
            {
               //if there is data but no summary, this blockfile needs summarizing.
               const auto &file = block.f;
               if (file->IsDataAvailable() && !file->IsSummaryAvailable())
               {
                  const auto odpcmaFile =
                     std::static_pointer_cast<ODPCMAliasBlockFile>(file);
                  odpcmaFile->SetStart(block.start);
                  odpcmaFile->SetClipOffset(sampleCount(
                     clip->GetStartTime()*clip->GetRate()
                  ));

                  //these will always be linear within a sequence-lets take advantage of this by keeping a cursor.
                  {
                     std::shared_ptr< ODPCMAliasBlockFile > ptr;
                     while(insertCursor < (int)tempBlocks.size() &&
                           (!(ptr = tempBlocks[insertCursor].lock()) ||
                            ptr->GetStart() + ptr->GetClipOffset() <
                            odpcmaFile->GetStart() + odpcmaFile->GetClipOffset()))
                        insertCursor++;
                  }

                  tempBlocks.insert(tempBlocks.begin() + insertCursor++, odpcmaFile);
               }
            }
         }
      }
   }
}

   //get the NEW order.
   OrderBlockFiles(tempBlocks);

   mHasUpdateRun = true;
}



///Computes the summary calculation queue order of the blockfiles
void ODComputeSummaryTask::OrderBlockFiles
   (std::vector< std::weak_ptr< ODPCMAliasBlockFile > > &unorderedBlocks)
{
   mBlockFiles.clear();
   //Order the blockfiles into our queue in a fancy convenient way.  (this could be user-prefs)
   //for now just put them in linear.  We start the order from the first block that includes the ondemand sample
   //(which the user sets by clicking.)
   //Note that this code assumes that the array is sorted in time.

   //find the startpoint
   auto processStartSample = GetDemandSample();
   std::shared_ptr< ODPCMAliasBlockFile > firstBlock;
   for (auto i = unorderedBlocks.size(); i--;)
   {
      auto ptr = unorderedBlocks[i].lock();
      if (ptr)
      {
         //test if the blockfiles are near the task cursor.  we use the last mBlockFiles[0] as our point of reference
         //and add ones that are closer.
         if (firstBlock &&
            ptr->GetGlobalEnd() >= processStartSample &&
                ( firstBlock->GetGlobalEnd() < processStartSample ||
                  ptr->GetGlobalStart() <= firstBlock->GetGlobalStart())
            )
         {
            //insert at the front of the list if we get blockfiles that are after the demand sample
            firstBlock = ptr;
            mBlockFiles.insert(mBlockFiles.begin(), unorderedBlocks[i]);
         }
         else
         {
            //otherwise no priority
            if ( !firstBlock )
               firstBlock = ptr;
            mBlockFiles.push_back(unorderedBlocks[i]);
         }
         if (mMaxBlockFiles< (int) mBlockFiles.size())
            mMaxBlockFiles = mBlockFiles.size();
      }
      else
      {
         // The block file disappeared.
         // Let it be deleted and forget about it.
      }
   }
}

#include "../Project.h"
#include "../UndoManager.h"
#include "ODManager.h"

namespace {

// Attach an object to each project.  It receives undo events and updates
// task queues.
struct ODTaskMaker final : ClientData::Base, wxEvtHandler
{
   AudacityProject &mProject;

   explicit ODTaskMaker( AudacityProject &project )
      : mProject{ project }
   {
      project.Bind(
         EVT_UNDO_RESET, &ODTaskMaker::OnUpdate, this );
      project.Bind(
         EVT_UNDO_OR_REDO, &ODTaskMaker::OnUpdate, this );
   }
   ODTaskMaker( const ODTaskMaker & ) PROHIBITED;
   ODTaskMaker &operator=( const ODTaskMaker & ) PROHIBITED;

   void OnUpdate( wxCommandEvent & e )
   {
      e.Skip();

      bool odUsed = false;
      std::unique_ptr<ODComputeSummaryTask> computeTask;
      auto &tracks = TrackList::Get( mProject );

      for (auto wt : tracks.Any<WaveTrack>())
      {
         //add the track to OD if the manager exists.  later we might do a more rigorous check...
         //if the ODManager hasn't been initialized, there's no chance this track has OD blocks since this
         //is a "Redo" operation.
         //TODO: update this to look like the update loop in OpenFile that handles general purpose ODTasks.
         //BUT, it is too slow to go thru every blockfile and check the odtype, so maybe put a flag in wavetrack
         //that gets unset on OD Completion, (and we could also update the drawing there too.)  The hard part is that
         //we would need to watch every possible way a OD Blockfile could get inserted into a wavetrack and change the
         //flag there.
         if(ODManager::IsInstanceCreated())
         {
            if(!odUsed)
            {
               computeTask = std::make_unique<ODComputeSummaryTask>();
               odUsed=true;
            }
            // PRL:  Is it correct to add all tracks to one task, even if they
            // are not partnered channels?  Rather than
            // make one task for each?
            computeTask->AddWaveTrack(wt->SharedPointer< WaveTrack >());
         }
      }

      //add the task.
      if(odUsed)
         ODManager::Instance()->AddNewTask(std::move(computeTask));
   }
};

static const AudacityProject::AttachedObjects::RegisteredFactory key{
  []( AudacityProject &project ){
     return std::make_shared< ODTaskMaker >( project );
   }
};

}
