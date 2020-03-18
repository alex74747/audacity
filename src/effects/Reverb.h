/**********************************************************************

   Audacity: A Digital Audio Editor
   Audacity(R) is copyright (c) 1999-2013 Audacity Team.
   License: GPL v2 or later.  See License.txt.

   Reverb.h
   Rob Sykes, Vaughan Johnson

**********************************************************************/

#ifndef __AUDACITY_EFFECT_REVERB__
#define __AUDACITY_EFFECT_REVERB__

#include "Effect.h"
#include "../ShuttleAutomation.h"

class ShuttleGui;

struct Reverb_priv_t;

class EffectReverb final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectReverb();
   virtual ~EffectReverb();

   struct Params
   {
      double mRoomSize;
      double mPreDelay;
      double mReverberance;
      double mHfDamping;
      double mToneLow;
      double mToneHigh;
      double mWetGain;
      double mDryGain;
      double mStereoWidth;
      bool mWetOnly;
   };

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   ManualPageID ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   RegistryPaths GetFactoryPresets() override;
   bool LoadFactoryPreset(int id) override;

   // EffectProcessor implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;
   bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) override;
   bool ProcessFinalize() override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;

   // Effect implementation

   bool Startup() override;
   void PopulateOrExchange(ShuttleGui & S) override;

private:
   // EffectReverb implementation

   void SetTitle(const wxString & name = {});

private:
   unsigned mNumChans {};
   Reverb_priv_t *mP;

   Params mParams;

   CapturedParameters mParameters;
   CapturedParameters &Parameters() override { return mParameters; }
};

#endif
