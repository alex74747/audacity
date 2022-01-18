/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.cpp
 
 Paul Licameli split from EffectManager.cpp
 
 **********************************************************************/


#include "RealtimeEffectManager.h"
#include "RealtimeEffectState.h"

#include "EffectInterface.h"
#include <memory>

#include <atomic>
#include <wx/time.h>

RealtimeEffectManager & RealtimeEffectManager::Get()
{
   static RealtimeEffectManager rem;
   return rem;
}

RealtimeEffectManager::RealtimeEffectManager()
{
   mRealtimeLock.Enter();
   mRealtimeActive = false;
   mRealtimeSuspended = true;
   mRealtimeLatency = 0;
   mRealtimeLock.Leave();
}

RealtimeEffectManager::~RealtimeEffectManager()
{
}

bool RealtimeEffectManager::RealtimeIsActive()
{
   return mStates.size() != 0;
}

bool RealtimeEffectManager::RealtimeIsSuspended()
{
   return mRealtimeSuspended;
}

void RealtimeEffectManager::RealtimeAddEffect(EffectProcessor &effect)
{
   // Block RealtimeProcess()
   SuspensionScope scope;

   // Add to list of active effects
   mStates.emplace_back( std::make_unique< RealtimeEffectState >( effect ) );
   auto &state = mStates.back();

   // Initialize effect if realtime is already active
   if (mRealtimeActive)
   {
      // Initialize realtime processing
      effect.RealtimeInitialize();

      // Add the required processors
      for (size_t i = 0, cnt = mRealtimeChans.size(); i < cnt; i++)
      {
         state->RealtimeAddProcessor(i, mRealtimeChans[i], mRealtimeRates[i]);
      }
   }
}

void RealtimeEffectManager::RealtimeRemoveEffect(EffectProcessor &effect)
{
   // Block RealtimeProcess()
   SuspensionScope scope;

   if (mRealtimeActive)
   {
      // Cleanup realtime processing
      effect.RealtimeFinalize();
   }
      
   // Remove from list of active effects
   auto end = mStates.end();
   auto found = std::find_if( mStates.begin(), end,
      [&](const decltype(mStates)::value_type &state){
         return &state->GetEffect() == &effect;
      }
   );
   if (found != end)
      mStates.erase(found);
}

void RealtimeEffectManager::RealtimeInitialize(double rate)
{
   // The audio thread should not be running yet, but protect anyway
   SuspensionScope scope;

   // (Re)Set processor parameters
   mRealtimeChans.clear();
   mRealtimeRates.clear();

   // RealtimeAdd/RemoveEffect() needs to know when we're active so it can
   // initialize newly added effects
   mRealtimeActive = true;

   // Tell each effect to get ready for action
   for (auto &state : mStates) {
      state->GetEffect().SetSampleRate(rate);
      state->GetEffect().RealtimeInitialize();
   }
}

void RealtimeEffectManager::RealtimeAddProcessor(int group, unsigned chans, float rate)
{
   for (auto &state : mStates)
      state->RealtimeAddProcessor(group, chans, rate);

   mRealtimeChans.push_back(chans);
   mRealtimeRates.push_back(rate);
}

void RealtimeEffectManager::RealtimeFinalize()
{
   // Make sure nothing is going on
   RealtimeSuspend();

   // It is now safe to clean up
   mRealtimeLatency = 0;

   // Tell each effect to clean up as well
   for (auto &state : mStates)
      state->GetEffect().RealtimeFinalize();

   // Reset processor parameters
   mRealtimeChans.clear();
   mRealtimeRates.clear();

   // No longer active
   mRealtimeActive = false;
}

void RealtimeEffectManager::RealtimeSuspend()
{
   mRealtimeLock.Enter();

   // Already suspended...bail
   if (mRealtimeSuspended)
   {
      mRealtimeLock.Leave();
      return;
   }

   // Show that we aren't going to be doing anything
   mRealtimeSuspended = true;

   // And make sure the effects don't either
   for (auto &state : mStates)
      state->RealtimeSuspend();

   mRealtimeLock.Leave();
}

void RealtimeEffectManager::RealtimeSuspendOne( EffectProcessor &effect )
{
   auto begin = mStates.begin(), end = mStates.end();
   auto found = std::find_if( begin, end,
      [&effect]( const decltype( mStates )::value_type &state ){
         return state && &state->GetEffect() == &effect;
      }
   );
   if ( found != end )
      (*found)->RealtimeSuspend();
}

void RealtimeEffectManager::RealtimeResume() noexcept
{
   mRealtimeLock.Enter();

   // Already running...bail
   if (!mRealtimeSuspended)
   {
      mRealtimeLock.Leave();
      return;
   }

   // Tell the effects to get ready for more action
   for (auto &state : mStates)
      state->RealtimeResume();

   // And we should too
   mRealtimeSuspended = false;

   mRealtimeLock.Leave();
}

void RealtimeEffectManager::RealtimeResumeOne( EffectProcessor &effect )
{
   auto begin = mStates.begin(), end = mStates.end();
   auto found = std::find_if( begin, end,
      [&effect]( const decltype( mStates )::value_type &state ){
         return state && &state->GetEffect() == &effect;
      }
   );
   if ( found != end )
      (*found)->RealtimeResume();
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::RealtimeProcessStart()
{
   // Protect ourselves from the main thread
   mRealtimeLock.Enter();

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended.
   if (!mRealtimeSuspended)
   {
      for (auto &state : mStates)
      {
         if (state->IsRealtimeActive())
            state->GetEffect().RealtimeProcessStart();
      }
   }

   mRealtimeLock.Leave();
}

//
// This will be called in a different thread than the main GUI thread.
//
size_t RealtimeEffectManager::RealtimeProcess(int group, unsigned chans, float **buffers, size_t numSamples)
{
   // Protect ourselves from the main thread
   mRealtimeLock.Enter();

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended, so allow the samples to pass as-is.
   if (mRealtimeSuspended || mStates.empty())
   {
      mRealtimeLock.Leave();
      return numSamples;
   }

   // Remember when we started so we can calculate the amount of latency we
   // are introducing
   wxMilliClock_t start = wxGetUTCTimeMillis();

   // Allocate the in/out buffer arrays
   float **ibuf = (float **) alloca(chans * sizeof(float *));
   float **obuf = (float **) alloca(chans * sizeof(float *));

   // And populate the input with the buffers we've been given while allocating
   // NEW output buffers
   for (unsigned int i = 0; i < chans; i++)
   {
      ibuf[i] = buffers[i];
      obuf[i] = (float *) alloca(numSamples * sizeof(float));
   }

   // Now call each effect in the chain while swapping buffer pointers to feed the
   // output of one effect as the input to the next effect
   size_t called = 0;
   for (auto &state : mStates)
   {
      if (state->IsRealtimeActive())
      {
         state->RealtimeProcess(group, chans, ibuf, obuf, numSamples);
         called++;
      }

      for (unsigned int j = 0; j < chans; j++)
      {
         float *temp;
         temp = ibuf[j];
         ibuf[j] = obuf[j];
         obuf[j] = temp;
      }
   }

   // Once we're done, we might wind up with the last effect storing its results
   // in the temporary buffers.  If that's the case, we need to copy it over to
   // the caller's buffers.  This happens when the number of effects processed
   // is odd.
   if (called & 1)
   {
      for (unsigned int i = 0; i < chans; i++)
      {
         memcpy(buffers[i], ibuf[i], numSamples * sizeof(float));
      }
   }

   // Remember the latency
   mRealtimeLatency = (int) (wxGetUTCTimeMillis() - start).GetValue();

   mRealtimeLock.Leave();

   //
   // This is wrong...needs to handle tails
   //
   return numSamples;
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::RealtimeProcessEnd() noexcept
{
   // Protect ourselves from the main thread
   mRealtimeLock.Enter();

   // Can be suspended because of the audio stream being paused or because effects
   // have been suspended.
   if (!mRealtimeSuspended)
   {
      for (auto &state : mStates)
      {
         if (state->IsRealtimeActive())
            state->GetEffect().RealtimeProcessEnd();
      }
   }

   mRealtimeLock.Leave();
}

int RealtimeEffectManager::GetRealtimeLatency()
{
   return mRealtimeLatency;
}
