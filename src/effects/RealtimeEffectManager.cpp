/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.cpp
 
 **********************************************************************/


#include "RealtimeEffectManager.h"
#include "RealtimeEffectList.h"
#include "RealtimeEffectState.h"

#include "EffectInterface.h"
#include "EffectHostInterface.h"
#include <memory>
#include "Project.h"
#include "../ProjectHistory.h"
#include "Track.h"
#include "UndoManager.h"

#include <atomic>

#include <wx/log.h>

static const AttachedProjectObjects::RegisteredFactory manager
{
   [](AudacityProject &project)
   {
      return std::make_shared<RealtimeEffectManager>(project);
   }
};

RealtimeEffectManager &RealtimeEffectManager::Get(AudacityProject &project)
{
   return project.AttachedObjects::Get<RealtimeEffectManager&>(manager);
}

const RealtimeEffectManager &RealtimeEffectManager::Get(const AudacityProject &project)
{
   return Get(const_cast<AudacityProject &>(project));
}

static ProjectFileIORegistry::ObjectReaderEntry registerFactory
{
   "effects",
   [](AudacityProject &project)
   {
      auto & manager = RealtimeEffectManager::Get(project);
      return manager.ReadXML(project);
   }
};

RealtimeEffectManager::RealtimeEffectManager(AudacityProject &project)
   : mProject(project)
{
}

RealtimeEffectManager::~RealtimeEffectManager()
{
}

bool RealtimeEffectManager::IsActive() const noexcept
{
   return mActive;
}

bool RealtimeEffectManager::IsSuspended() const
{
   return mSuspended;
}

void RealtimeEffectManager::Initialize(double rate)
{
   wxLogDebug(wxT("Initialize"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   wxASSERT(!mActive);
   wxASSERT(!mSuspended);

   // The audio thread should not be running yet, but protect anyway
   SuspensionScope scope{ &mProject };

   // Remember the rate
   mRate = rate;

   // (Re)Set processor parameters
   mChans.clear();
   mRates.clear();
   mGroupLeaders.clear();

   // RealtimeAdd/RemoveEffect() needs to know when we're active so it can
   // initialize newly added effects
   mActive = true;
}

void RealtimeEffectManager::AddProcessor(Track *track, unsigned chans, float rate)
{
   wxLogDebug(wxT("AddProcess"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   wxASSERT(mActive);
   wxASSERT(!mSuspended);

   SuspensionScope scope{ &mProject };

   auto leader = *track->GetOwner()->FindLeader(track);
   mGroupLeaders.push_back(leader);
   mChans.insert({leader, chans});
   mRates.insert({leader, rate});

   VisitGroup(leader,
      [&](RealtimeEffectState & state, bool bypassed)
      {
         state.Initialize(rate);

         state.AddProcessor(leader, chans, rate);
      }
   );
}

void RealtimeEffectManager::Finalize()
{
   wxLogDebug(wxT("Finalize"));

   wxASSERT(mActive);
   wxASSERT(!mProcessing);
   wxASSERT(!mSuspended);

   while (mProcessing.load())
   {
      wxMilliSleep(1);
   }

   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   wxASSERT(mActive);

   // Make sure nothing is going on
   SuspensionScope scope{ &mProject };

   // It is now safe to clean up
   mLatency = std::chrono::microseconds(0);

   // Process master list
   auto & states = RealtimeEffectList::Get(mProject).GetStates();
   for (auto & state : states)
   {
      state->Finalize();
   }

   // And all track lists
   for (auto leader : mGroupLeaders)
   {
      auto & states = RealtimeEffectList::Get(*leader).GetStates();
      for (auto &state : states)
      {
         state->Finalize();
      }
   }

   // Reset processor parameters
   mGroupLeaders.clear();
   mChans.clear();
   mRates.clear();

   // No longer active
   mActive = false;
}

void RealtimeEffectManager::Suspend()
{
   wxLogDebug(wxT("Suspend"));
   // Already suspended...bail
   if (mSuspended)
   {
      return;
   }

   // Show that we aren't going to be doing anything
   mSuspended = true;

   auto & masterList = RealtimeEffectList::Get(mProject);
   masterList.Suspend();

   for (auto leader : mGroupLeaders)
   {
      auto & trackList = RealtimeEffectList::Get(*leader);
      trackList.Suspend();
   }
}

void RealtimeEffectManager::Resume() noexcept
{
   wxLogDebug(wxT("Resume"));
   // Already running...bail
   if (!mSuspended)
   {
      return;
   }
 
   auto & masterList = RealtimeEffectList::Get(mProject);
   masterList.Resume();

   for (auto leader : mGroupLeaders)
   {
      auto & trackList = RealtimeEffectList::Get(*leader);
      trackList.Resume();
   }

   // Show that we aren't going to be doing anything
   mSuspended = false;
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::ProcessStart()
{
   wxLogDebug(wxT("ProcessStart"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   wxASSERT(mActive);
   wxASSERT(!mProcessing);

   SuspensionScope scope{ &mProject };

   mCurrentGroup = 0;

   for (auto leader : mGroupLeaders)
   {
      VisitGroup(leader,
         [&](RealtimeEffectState &state, bool bypassed)
         {
            state.ProcessStart();
         }
      );
   }

   mProcessing = true;
}

//
// This will be called in a different thread than the main GUI thread.
//
size_t RealtimeEffectManager::Process(Track *track,
                                      float gain,
                                      float **buffers,
                                      size_t numSamps)
{
   wxLogDebug(wxT("Process"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   if (mSuspended || !mProcessing)
   {
      return 0;
   }

   auto numChans = mChans[track];

   // Remember when we started so we can calculate the amount of latency we
   // are introducing
   auto start = std::chrono::steady_clock::now();

   // Allocate the in, out, and prefade buffer arrays
   float **ibuf = (float **) alloca(numChans * sizeof(float *));
   float **obuf = (float **) alloca(numChans * sizeof(float *));
   float **pibuf = (float **) alloca(numChans * sizeof(float *));
   float **pobuf = (float **) alloca(numChans * sizeof(float *));

   auto group = mCurrentGroup++;

   auto prefade = HasPrefaders(group);

   // And populate the input with the buffers we've been given while allocating
   // NEW output buffers
   for (unsigned int c = 0; c < numChans; ++c)
   {
      ibuf[c] = buffers[c];
      obuf[c] = (float *) alloca(numSamps * sizeof(float));

      if (prefade)
      {
         pibuf[c] = (float *) alloca(numSamps * sizeof(float));
         pobuf[c] = (float *) alloca(numSamps * sizeof(float));

         memcpy(pibuf[c], buffers[c], numSamps * sizeof(float));
      }
   }

   // Apply gain
   if (gain != 1.0)
   {
      for (auto c = 0; c < numChans; ++c)
      {
         for (auto s = 0; s < numSamps; ++s)
         {
            ibuf[c][s] *= gain;
         }
      }
   }

   // Now call each effect in the chain while swapping buffer pointers to feed the
   // output of one effect as the input to the next effect
   // Tracks how many processors were called
   size_t called = 0;
   if (HasPostfaders(group))
   {
      VisitGroup(track,
         [&](RealtimeEffectState &state, bool bypassed)
         {
            if (bypassed || state.IsPreFade())
            {
               return;
            }

            state.Process(track, numChans, ibuf, obuf, numSamps);

            for (auto c = 0; c < numChans; ++c)
            {
               std::swap(ibuf[c], obuf[c]);
            }
            called++;
         }
      );

      // Once we're done, we might wind up with the last effect storing its results
      // in the temporary buffers.  If that's the case, we need to copy it over to
      // the caller's buffers.  This happens when the number of effects processed
      // is odd.
      if (called & 1)
      {
         for (auto c = 0; c < numChans; ++c)
         {
            memcpy(buffers[c], ibuf[c], numSamps * sizeof(float));
         }
      }
   }

   if (prefade)
   {
      VisitGroup(track,
         [&](RealtimeEffectState &state, bool bypassed)
         {
            if (bypassed || !state.IsPreFade())
            {
               return;
            }

            state.Process(track, numChans, pibuf, pobuf, numSamps);

            for (auto c = 0; c < numChans; ++c)
            {
               for (auto s = 0; s < numSamps; ++s)
               {
                  buffers[c][s] += pobuf[c][s];
               }

               std::swap(pibuf[c], pobuf[c]);
            }

            called++;
         }
      );
   }

   // Remember the latency
   auto end = std::chrono::steady_clock::now();
   mLatency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

   //
   // This is wrong...needs to handle tails
   //
   return called ? numSamps : 0;
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::ProcessEnd() noexcept
{
   wxLogDebug(wxT("ProcessEnd"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   if (!mProcessing)
   {
      return;
   }

   SuspensionScope scope{ &mProject };

   for (auto leader : mGroupLeaders)
   {
      VisitGroup(leader,
         [&](RealtimeEffectState &state, bool bypassed)
         {
            state.ProcessEnd();
         }
      );
   }

   mProcessing = false;
}


void RealtimeEffectManager::Show(AudacityProject &project)
{
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   auto & list = RealtimeEffectList::Get(project);
   list.Show(this, XO("Master Effects"));
}

void RealtimeEffectManager::Show(Track &track, wxPoint pos)
{
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   auto & list = RealtimeEffectList::Get(track);
   list.Show(this, XO("%s Effects").Format(track.GetName()), pos);
}

bool RealtimeEffectManager::IsBypassed(const Track &track)
{
   return RealtimeEffectList::Get(track).IsBypassed();
}

void RealtimeEffectManager::Bypass(Track &track, bool bypass)
{
   RealtimeEffectList::Get(track).Bypass(bypass);
}

bool RealtimeEffectManager::HasPrefaders(int group)
{
   return RealtimeEffectList::Get(mProject).HasPrefaders() ||
          RealtimeEffectList::Get(*mGroupLeaders[group]).HasPrefaders();
}

bool RealtimeEffectManager::HasPostfaders(int group)
{
   return RealtimeEffectList::Get(mProject).HasPostfaders() ||
          RealtimeEffectList::Get(*mGroupLeaders[group]).HasPostfaders();
}

void RealtimeEffectManager::VisitGroup(Track *leader,
   std::function<void(RealtimeEffectState &state, bool bypassed)> func)
{
   // Call the function for each effect on the master list
   RealtimeEffectList::Get(mProject).Visit(func);

   // Call the function for each effect on the track list
   if (leader)
     RealtimeEffectList::Get(*leader).Visit(func);
}

RealtimeEffectState &RealtimeEffectManager::AddState(RealtimeEffectList &states, const PluginID & id)
{
   wxLogDebug(wxT("AddState"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   // Block processing
   Suspend();

   auto & state = states.AddState(id);
   
   if (mActive)
   {
      state.Initialize(mRate);

      for (auto &leader : mGroupLeaders)
      {
         auto chans = mChans[leader];
         auto rate = mRates[leader];

         state.AddProcessor(leader, chans, rate);
      }
   }

   if (mProcessing)
   {
      state.ProcessStart();
   }

   ProjectHistory::Get(mProject).PushState(
      XO("Added %s effect").Format(state.GetEffect()->GetName()),
      XO("Added Effect"),
      UndoPush::NONE
   );

   // Allow RealtimeProcess() to, well, process 
   Resume();

   return state;
}

void RealtimeEffectManager::RemoveState(RealtimeEffectList &states, RealtimeEffectState &state)
{
   wxLogDebug(wxT("RemoveState"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   // Block RealtimeProcess()
   Suspend();

   ProjectHistory::Get(mProject).PushState(
      XO("Removed %s effect").Format(state.GetEffect()->GetName()),
      XO("Removed Effect"),
      UndoPush::NONE
   );

   if (mProcessing)
   {
      state.ProcessEnd();
   }

   if (mActive)
   {
      state.Finalize();
   }

   states.RemoveState(state);

   // Allow RealtimeProcess() to, well, process 
   Resume();
}

XMLTagHandler *RealtimeEffectManager::ReadXML(AudacityProject &project)
{
   return &RealtimeEffectList::Get(project);
}

XMLTagHandler *RealtimeEffectManager::ReadXML(Track &track)
{
   return &RealtimeEffectList::Get(track);
}

void RealtimeEffectManager::WriteXML(
   XMLWriter &xmlFile, const AudacityProject &project) const
{
   RealtimeEffectList::Get(project).WriteXML(xmlFile);
}

void RealtimeEffectManager::WriteXML(
   XMLWriter &xmlFile, const Track &track) const
{
   RealtimeEffectList::Get(track).WriteXML(xmlFile);
}

void RealtimeEffectManager::WriteXML(
   XMLWriter &xmlFile, const RealtimeEffectList &states) const
{
}

static ProjectFileIORegistry::ObjectWriterEntry entry {
[](const AudacityProject &project, XMLWriter &xmlFile){
   auto &realtimeEffectManager = RealtimeEffectManager::Get(project);
   realtimeEffectManager.WriteXML(xmlFile, project);
}
};

auto RealtimeEffectManager::GetLatency() const -> Latency
{
   return mLatency;
}
