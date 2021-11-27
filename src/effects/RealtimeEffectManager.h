/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.h
 
 **********************************************************************/

#ifndef __AUDACITY_REALTIME_EFFECT_MANAGER__
#define __AUDACITY_REALTIME_EFFECT_MANAGER__

#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "audacity/Types.h" // for PluginID
#include "ClientData.h"

class AudacityProject;
class EffectProcessor;
class RealtimeEffectList;
class RealtimeEffectState;
class Track;

class wxPoint;

class AUDACITY_DLL_API RealtimeEffectManager final
   : public ClientData::Base
   , public std::enable_shared_from_this<RealtimeEffectManager>
{
public:
   using Latency = std::chrono::microseconds;

   RealtimeEffectManager(AudacityProject &project);
   ~RealtimeEffectManager();

   static RealtimeEffectManager & Get(AudacityProject &project);
   static const RealtimeEffectManager & Get(const AudacityProject &project);

   bool IsBypassed(const Track &track);
   void Bypass(Track &track, bool bypass);

   bool IsActive() const noexcept;
   bool IsSuspended() const;
   void Initialize(double rate);
   void AddProcessor(Track *track, unsigned chans, float rate);
   void Finalize();
   void Suspend();
   void Resume() noexcept;
   Latency GetLatency() const;

   //! Object whose lifetime encompasses one suspension of processing in one thread
   class SuspensionScope {
   public:
      explicit SuspensionScope(AudacityProject *pProject)
         : mpProject{ pProject }
      {
         if (mpProject)
            Get(*mpProject).Suspend();
      }
      SuspensionScope( SuspensionScope &&other )
      {
         other.mpProject = nullptr;
      }
      SuspensionScope& operator=( SuspensionScope &&other )
      {
         auto pProject = other.mpProject;
         other.mpProject = nullptr;
         mpProject = pProject;
         return *this;
      }
      ~SuspensionScope()
      {
         if (mpProject)
            Get(*mpProject).Resume();
      }

   private:
      AudacityProject *mpProject = nullptr;
   };

   //! Object whose lifetime encompasses one block of processing in one thread
   class ProcessScope {
   public:
      explicit ProcessScope(AudacityProject *pProject)
         : mpProject{ pProject }
      {
         if (mpProject)
            Get(*mpProject).ProcessStart();
      }
      ProcessScope( ProcessScope &&other )
         : mpProject{ other.mpProject }
      {
         other.mpProject = nullptr;
      }
      ProcessScope& operator=( ProcessScope &&other )
      {
         auto pProject = other.mpProject;
         other.mpProject = nullptr;
         mpProject = pProject;
         return *this;
      }
      ~ProcessScope()
      {
         if (mpProject)
            Get(*mpProject).ProcessEnd();
      }

      size_t Process(Track *track, float gain, float **buffers, size_t numSamps)
      {
         if (mpProject)
            return Get(*mpProject).Process(track, gain, buffers, numSamps);
         return numSamps; // consider them trivially processed
      }

   private:
      AudacityProject *mpProject = nullptr;
   };

   void Show(AudacityProject &project);
   void Show(Track &track, wxPoint pos);

   AudacityProject &GetProject() { return mProject; }

   RealtimeEffectState & AddState(RealtimeEffectList &states, const PluginID & id);
   void RemoveState(RealtimeEffectList &states, RealtimeEffectState &state);

private:
   void ProcessStart();
   size_t Process(Track *track, float gain, float **buffers, size_t numSamps);
   void ProcessEnd() noexcept;

   RealtimeEffectManager(const RealtimeEffectManager&) = delete;
   RealtimeEffectManager &operator=(const RealtimeEffectManager&) = delete;

   // Visit the per-project states first, then any per-track states.
   void VisitGroup(Track *leader,
      std::function<void(RealtimeEffectState &state, bool bypassed)> func);

   AudacityProject &mProject;

   std::mutex mLock;
   Latency mLatency{ 0 };

   double mRate;

   std::atomic<bool> mSuspended{ true };
   std::atomic<bool> mActive{ false };
   std::atomic<bool> mProcessing{ false };

   std::vector<Track *> mGroupLeaders;
   std::unordered_map<Track *, int> mChans;
   std::unordered_map<Track *, double> mRates;
   int mCurrentGroup;
};

#endif
