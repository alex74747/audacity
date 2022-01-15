/**********************************************************************

  Audacity: A Digital Audio Editor

  DtmfGen.h

  Salvo Ventura
  Dec 2006

  An effect that generates DTMF tones

**********************************************************************/

#ifndef __AUDACITY_EFFECT_DTMF__
#define __AUDACITY_EFFECT_DTMF__

#include "Effect.h"
#include "../ShuttleAutomation.h"

class ShuttleGui;

class EffectDtmf final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectDtmf();
   virtual ~EffectDtmf();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   ManualPageID ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // EffectProcessor implementation

   unsigned GetAudioOutCount() override;
   bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;

   // Effect implementation

   bool Startup() override;
   bool Init() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataFromWindow() override;
   bool TransferDataToWindow() override;

private:
   // EffectDtmf implementation

   bool MakeDtmfTone(float *buffer, size_t len, float fs,
                     wxChar tone, sampleCount last,
                     sampleCount total, float amplitude);
   void Recalculate();

private:
   sampleCount numSamplesSequence;  // total number of samples to generate
   sampleCount numSamplesTone;      // number of samples in a tone block
   sampleCount numSamplesSilence;   // number of samples in a silence block
   sampleCount diff;                // number of extra samples to redistribute
   sampleCount numRemaining;        // number of samples left to produce in the current block
   sampleCount curTonePos;          // position in tone to start the wave
   bool isTone;                     // true if block is tone, otherwise silence
   int curSeqPos;                   // index into dtmf tone string

   wxString dtmfSequence;             // dtmf tone string
   int    dtmfNTones;               // total number of tones to generate
   double dtmfTone;                 // duration of a single tone in ms
   double dtmfSilence;              // duration of silence between tones in ms
   double dtmfDutyCycle;            // ratio of dtmfTone/(dtmfTone+dtmfSilence)
   double dtmfAmplitude;            // amplitude of dtmf tone sequence, restricted to (0-1)

   CapturedParameters mParameters;
   CapturedParameters &Parameters() override { return mParameters; }
};

#endif
