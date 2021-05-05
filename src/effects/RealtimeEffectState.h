/**********************************************************************

 Audacity: A Digital Audio Editor

 @file RealtimeEffectState.h

 Paul Licameli split from RealtimeEffectManager.cpp

 *********************************************************************/

#ifndef __AUDACITY_REALTIMEEFFECTSTATE_H__
#define __AUDACITY_REALTIMEEFFECTSTATE_H__

#include <atomic>
#include <unordered_map>
#include <vector>

#include "audacity/Types.h"

class EffectProcessor;
class Effect;
class Track;

class RealtimeEffectState
{
public:
   explicit RealtimeEffectState();
   explicit RealtimeEffectState(const PluginID & id);
   ~RealtimeEffectState();

   void SetID(const PluginID & id);
   EffectProcessor *GetEffect();

   bool Suspend();
   bool Resume() noexcept;

   bool Initialize(double rate);
   bool AddProcessor(Track *track, unsigned chans, float rate);
   bool ProcessStart();
   size_t Process(Track *track, unsigned chans, float **inbuf,  float **outbuf, size_t numSamples);
   bool ProcessEnd();
   bool IsActive() const noexcept;
   bool Finalize();

private:
   PluginID mID;
   std::unique_ptr<EffectProcessor> mEffect;

   std::unordered_map<Track *, int> mGroups;


   std::atomic<int> mRealtimeSuspendCount{ 1 };    // Effects are initially suspended
};

#endif // __AUDACITY_REALTIMEEFFECTSTATE_H__

