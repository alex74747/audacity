/**********************************************************************

  Audacity: A Digital Audio Editor

  Normalize.h

  Dominic Mazzoni
  Vaughan Johnson (Preview)

**********************************************************************/

#ifndef __AUDACITY_EFFECT_NORMALIZE__
#define __AUDACITY_EFFECT_NORMALIZE__

#include "Effect.h"
#include "Biquad.h"
#include "../ShuttleAutomation.h"

class ShuttleGui;

class EffectNormalize final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectNormalize();
   virtual ~EffectNormalize();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   ManualPageID ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // Effect implementation

   bool CheckWhetherSkipEffect() override;
   bool Startup() override;
   bool Process() override;
   void PopulateOrExchange(ShuttleGui & S) override;

private:
   // EffectNormalize implementation

   bool ProcessOne(
      WaveTrack * t, const TranslatableString &msg, unsigned count, float offset);
   bool AnalyseTrack(const WaveTrack * track, const TranslatableString &msg,
                     unsigned count, float &offset, float &extent);
   bool AnalyseTrackData(const WaveTrack * track, const TranslatableString &msg, unsigned count,
                     float &offset);
   void AnalyseDataDC(float *buffer, size_t len);
   void ProcessData(float *buffer, size_t len, float offset);

   bool CanApply() override;

private:
   double mPeakLevel;
   bool   mGain;
   bool   mDC;
   bool   mStereoInd;

   double mCurT0;
   double mCurT1;
   float  mMult;
   double mSum;

   bool mCreating;

   CapturedParameters mParameters;
   CapturedParameters& Parameters() override { return mParameters; }
};

#endif
