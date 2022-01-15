/**********************************************************************

  Audacity: A Digital Audio Editor

  Phaser.cpp

  Effect programming:
  Nasca Octavian Paul (Paul Nasca)

  UI programming:
  Dominic Mazzoni (with the help of wxDesigner)
  Vaughan Johnson (Preview)

*******************************************************************//**

\class EffectPhaser
\brief An Effect that changes frequencies in a time varying manner.

*//*******************************************************************/



#include "Phaser.h"
#include "LoadEffects.h"

#include <math.h>

#include <wx/slider.h>

#include "../Shuttle.h"
#include "../ShuttleGui.h"
#include "../widgets/valnum.h"

enum
{
   ID_Stages = 10000,
   ID_DryWet,
   ID_Freq,
   ID_Phase,
   ID_Depth,
   ID_Feedback,
   ID_OutGain
};

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key               Def   Min   Max         Scale
static auto Stages = Parameter<int>(
                           L"Stages",     2,    2,    NUM_STAGES, 1  );
static auto DryWet = Parameter<int>(
                           L"DryWet",     128,  0,    255,        1  );
static auto Freq = Parameter<double>(
                           L"Freq",       0.4,  0.001,4.0,        10.0 );
static auto Phase = Parameter<double>(
                           L"Phase",      0.0,  0.0,  360.0,      1  );
static auto Depth = Parameter<int>(
                           L"Depth",      100,  0,    255,        1  );
static auto Feedback = Parameter<int>(
                           L"Feedback",   0,    -100, 100,        1  );
static auto OutGain = Parameter<double>(
                           L"Gain",      -6.0,    -30.0,    30.0,    1   );

//
#define phaserlfoshape 4.0

// How many samples are processed before recomputing the lfo value again
#define lfoskipsamples 20

//
// EffectPhaser
//

const ComponentInterfaceSymbol EffectPhaser::Symbol
{ XO("Phaser") };

namespace{ BuiltinEffectsModule::Registration< EffectPhaser > reg; }

BEGIN_EVENT_TABLE(EffectPhaser, wxEvtHandler)
    EVT_SLIDER(ID_Stages, EffectPhaser::OnStagesSlider)
    EVT_SLIDER(ID_DryWet, EffectPhaser::OnDryWetSlider)
    EVT_SLIDER(ID_Freq, EffectPhaser::OnFreqSlider)
    EVT_SLIDER(ID_Phase, EffectPhaser::OnPhaseSlider)
    EVT_SLIDER(ID_Depth, EffectPhaser::OnDepthSlider)
    EVT_SLIDER(ID_Feedback, EffectPhaser::OnFeedbackSlider)
    EVT_SLIDER(ID_OutGain, EffectPhaser::OnGainSlider)
    EVT_TEXT(ID_Stages, EffectPhaser::OnStagesText)
    EVT_TEXT(ID_DryWet, EffectPhaser::OnDryWetText)
    EVT_TEXT(ID_Freq, EffectPhaser::OnFreqText)
    EVT_TEXT(ID_Phase, EffectPhaser::OnPhaseText)
    EVT_TEXT(ID_Depth, EffectPhaser::OnDepthText)
    EVT_TEXT(ID_Feedback, EffectPhaser::OnFeedbackText)
    EVT_TEXT(ID_OutGain, EffectPhaser::OnGainText)
END_EVENT_TABLE()

EffectPhaser::EffectPhaser()
{
   mStages = Stages.def;
   mDryWet = DryWet.def;
   mFreq = Freq.def;
   mPhase = Phase.def;
   mDepth = Depth.def;
   mFeedback = Feedback.def;
   mOutGain = OutGain.def;

   SetLinearEffectFlag(true);
}

EffectPhaser::~EffectPhaser()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectPhaser::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectPhaser::GetDescription()
{
   return XO("Combines phase-shifted signals with the original signal");
}

ManualPageID EffectPhaser::ManualPage()
{
   return L"Phaser";
}

// EffectDefinitionInterface implementation

EffectType EffectPhaser::GetType()
{
   return EffectTypeProcess;
}

bool EffectPhaser::SupportsRealtime()
{
#if defined(EXPERIMENTAL_REALTIME_AUDACITY_EFFECTS)
   return true;
#else
   return false;
#endif
}

// EffectProcessor implementation

unsigned EffectPhaser::GetAudioInCount()
{
   return 1;
}

unsigned EffectPhaser::GetAudioOutCount()
{
   return 1;
}

bool EffectPhaser::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames chanMap)
{
   InstanceInit(mMaster, mSampleRate);
   if (chanMap[0] == ChannelNameFrontRight)
   {
      mMaster.phase += M_PI;
   }

   return true;
}

size_t EffectPhaser::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   return InstanceProcess(mMaster, inBlock, outBlock, blockLen);
}

bool EffectPhaser::RealtimeInitialize()
{
   SetBlockSize(512);

   mSlaves.clear();

   return true;
}

bool EffectPhaser::RealtimeAddProcessor(unsigned WXUNUSED(numChannels), float sampleRate)
{
   EffectPhaserState slave;

   InstanceInit(slave, sampleRate);

   mSlaves.push_back(slave);

   return true;
}

bool EffectPhaser::RealtimeFinalize()
{
   mSlaves.clear();

   return true;
}

size_t EffectPhaser::RealtimeProcess(int group,
                                          float **inbuf,
                                          float **outbuf,
                                          size_t numSamples)
{

   return InstanceProcess(mSlaves[group], inbuf, outbuf, numSamples);
}
bool EffectPhaser::DefineParams( ShuttleParams & S ){
   S.SHUTTLE_PARAM( mStages,    Stages );
   S.SHUTTLE_PARAM( mDryWet,    DryWet );
   S.SHUTTLE_PARAM( mFreq,      Freq );
   S.SHUTTLE_PARAM( mPhase,     Phase );
   S.SHUTTLE_PARAM( mDepth,     Depth );
   S.SHUTTLE_PARAM( mFeedback,  Feedback );
   S.SHUTTLE_PARAM( mOutGain,   OutGain );
   return true;
}

bool EffectPhaser::GetAutomationParameters(CommandParameters & parms)
{
   parms.Write(Stages.key, mStages);
   parms.Write(DryWet.key, mDryWet);
   parms.Write(Freq.key, mFreq);
   parms.Write(Phase.key, mPhase);
   parms.Write(Depth.key, mDepth);
   parms.Write(Feedback.key, mFeedback);
   parms.Write(OutGain.key, mOutGain);

   return true;
}

bool EffectPhaser::SetAutomationParameters(CommandParameters & parms)
{
   ReadParam(Stages);
   ReadParam(DryWet);
   ReadParam(Freq);
   ReadParam(Phase);
   ReadParam(Depth);
   ReadParam(Feedback);
   ReadParam(OutGain);

   if (Stages & 1)    // must be even, but don't complain about it
   {
      Stages.cache &= ~1;
   }

   mFreq = Freq;
   mFeedback = Feedback;
   mStages = Stages;
   mDryWet = DryWet;
   mDepth = Depth;
   mPhase = Phase;
   mOutGain = OutGain;

   return true;
}

// Effect implementation

void EffectPhaser::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(5);
   S.AddSpace(0, 5);

   S.StartMultiColumn(3, wxEXPAND);
   {
      S.SetStretchyCol(2);

      mStagesT =
      S
         .Id(ID_Stages)
         .Validator<IntegerValidator<int>>(
            &mStages, NumValidatorStyle::DEFAULT, Stages.min, Stages.max)
         .AddTextBox(XXO("&Stages:"), L"", 15);

      mStagesS =
      S
         .Id(ID_Stages)
         .Text(XO("Stages"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, Stages.def * Stages.scale, Stages.max * Stages.scale, Stages.min * Stages.scale,
            2 /* line size */ );

      mDryWetT =
      S
         .Id(ID_DryWet)
         .Validator<IntegerValidator<int>>(
            &mDryWet, NumValidatorStyle::DEFAULT, DryWet.min, DryWet.max)
         .AddTextBox(XXO("&Dry/Wet:"), L"", 15);

      mDryWetS =
      S
         .Id(ID_DryWet)
         .Text(XO("Dry Wet"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, DryWet.def * DryWet.scale, DryWet.max * DryWet.scale, DryWet.min * DryWet.scale);

      mFreqT =
      S
         .Id(ID_Freq)
         .Validator<FloatingPointValidator<double>>(
            5, &mFreq, NumValidatorStyle::ONE_TRAILING_ZERO, Freq.min, Freq.max)
         .AddTextBox(XXO("LFO Freq&uency (Hz):"), L"", 15);

      mFreqS =
      S
         .Id(ID_Freq)
         .Text(XO("LFO frequency in hertz"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, Freq.def * Freq.scale, Freq.max * Freq.scale, 0.0);

      mPhaseT =
      S
         .Id(ID_Phase)
         .Validator<FloatingPointValidator<double>>(
            1, &mPhase, NumValidatorStyle::DEFAULT, Phase.min, Phase.max)
         .AddTextBox(XXO("LFO Sta&rt Phase (deg.):"), L"", 15);

      mPhaseS =
      S
         .Id(ID_Phase)
         .Text(XO("LFO start phase in degrees"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, Phase.def * Phase.scale, Phase.max * Phase.scale, Phase.min * Phase.scale,
            10 /* line size */ );

      mDepthT =
      S
         .Id(ID_Depth)
         .Validator<IntegerValidator<int>>(
            &mDepth, NumValidatorStyle::DEFAULT, Depth.min, Depth.max)
         .AddTextBox(XXO("Dept&h:"), L"", 15);

      mDepthS =
      S
         .Id(ID_Depth)
         .Text(XO("Depth in percent"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, Depth.def * Depth.scale, Depth.max * Depth.scale, Depth.min * Depth.scale);

      mFeedbackT =
      S
         .Id(ID_Feedback)
         .Validator<IntegerValidator<int>>(
            &mFeedback, NumValidatorStyle::DEFAULT, Feedback.min, Feedback.max)
         .AddTextBox(XXO("Feedbac&k (%):"), L"", 15);

      mFeedbackS =
      S
         .Id(ID_Feedback)
         .Text(XO("Feedback in percent"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, Feedback.def * Feedback.scale, Feedback.max * Feedback.scale, Feedback.min * Feedback.scale,
            10 /* line size */ );

      mOutGainT =
      S
         .Id(ID_OutGain)
         .Validator<FloatingPointValidator<double>>(
            1, &mOutGain, NumValidatorStyle::DEFAULT, OutGain.min, OutGain.max)
         .AddTextBox(XXO("&Output gain (dB):"), L"", 12);

      mOutGainS =
      S
         .Id(ID_OutGain)
         .Text(XO("Output gain (dB)"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, OutGain.def * OutGain.scale, OutGain.max * OutGain.scale, OutGain.min * OutGain.scale);
   }
   S.EndMultiColumn();
}

bool EffectPhaser::TransferDataToWindow()
{
   if (!mUIParent->TransferDataToWindow())
   {
      return false;
   }

   mStagesS->SetValue((int) (mStages * Stages.scale));
   mDryWetS->SetValue((int) (mDryWet * DryWet.scale));
   mFreqS->SetValue((int) (mFreq * Freq.scale));
   mPhaseS->SetValue((int) (mPhase * Phase.scale));
   mDepthS->SetValue((int) (mDepth * Depth.scale));
   mFeedbackS->SetValue((int) (mFeedback * Feedback.scale));
   mOutGainS->SetValue((int) (mOutGain * OutGain.scale));

   return true;
}

bool EffectPhaser::TransferDataFromWindow()
{
   if (!mUIParent->Validate() || !mUIParent->TransferDataFromWindow())
   {
      return false;
   }

   if (mStages & 1)    // must be even
   {
      mStages &= ~1;
      mStagesT->GetValidator()->TransferToWindow();
   }

   return true;
}

// EffectPhaser implementation

void EffectPhaser::InstanceInit(EffectPhaserState & data, float sampleRate)
{
   data.samplerate = sampleRate;

   for (int j = 0; j < mStages; j++)
   {
      data.old[j] = 0;
   }

   data.skipcount = 0;
   data.gain = 0;
   data.fbout = 0;
   data.laststages = 0;
   data.outgain = 0;

   return;
}

size_t EffectPhaser::InstanceProcess(EffectPhaserState & data, float **inBlock, float **outBlock, size_t blockLen)
{
   float *ibuf = inBlock[0];
   float *obuf = outBlock[0];

   for (int j = data.laststages; j < mStages; j++)
   {
      data.old[j] = 0;
   }
   data.laststages = mStages;

   data.lfoskip = mFreq * 2 * M_PI / data.samplerate;
   data.phase = mPhase * M_PI / 180;
   data.outgain = DB_TO_LINEAR(mOutGain);

   for (decltype(blockLen) i = 0; i < blockLen; i++)
   {
      double in = ibuf[i];

      double m = in + data.fbout * mFeedback / 101;  // Feedback must be less than 100% to avoid infinite gain.

      if (((data.skipcount++) % lfoskipsamples) == 0)
      {
         //compute sine between 0 and 1
         data.gain =
            (1.0 +
             cos(data.skipcount.as_double() * data.lfoskip
                 + data.phase)) / 2.0;

         // change lfo shape
         data.gain = expm1(data.gain * phaserlfoshape) / expm1(phaserlfoshape);

         // attenuate the lfo
         data.gain = 1.0 - data.gain / 255.0 * mDepth;
      }

      // phasing routine
      for (int j = 0; j < mStages; j++)
      {
         double tmp = data.old[j];
         data.old[j] = data.gain * tmp + m;
         m = tmp - data.gain * data.old[j];
      }
      data.fbout = m;

      obuf[i] = (float) (data.outgain * (m * mDryWet + in * (255 - mDryWet)) / 255);
   }

   return blockLen;
}

void EffectPhaser::OnStagesSlider(wxCommandEvent & evt)
{
   mStages = (evt.GetInt() / Stages.scale) & ~1;  // must be even;
   mStagesT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectPhaser::OnDryWetSlider(wxCommandEvent & evt)
{
   mDryWet = evt.GetInt() / DryWet.scale;
   mDryWetT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectPhaser::OnFreqSlider(wxCommandEvent & evt)
{
   mFreq = (double) evt.GetInt() / Freq.scale;
   if (mFreq < Freq.min) mFreq = Freq.min;
   mFreqT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectPhaser::OnPhaseSlider(wxCommandEvent & evt)
{
   int val = ((evt.GetInt() + 5) / 10) * 10; // round to nearest multiple of 10
   val = val > Phase.max * Phase.scale ? Phase.max * Phase.scale : val;
   mPhaseS->SetValue(val);
   mPhase =  (double) val / Phase.scale;
   mPhaseT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectPhaser::OnDepthSlider(wxCommandEvent & evt)
{
   mDepth = evt.GetInt() / Depth.scale;
   mDepthT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectPhaser::OnFeedbackSlider(wxCommandEvent & evt)
{
   int val = evt.GetInt();
   val = ((val + (val > 0 ? 5 : -5)) / 10) * 10; // round to nearest multiple of 10
   val = val > Feedback.max * Feedback.scale ? Feedback.max * Feedback.scale : val;
   mFeedbackS->SetValue(val);
   mFeedback = val / Feedback.scale;
   mFeedbackT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectPhaser::OnGainSlider(wxCommandEvent & evt)
{
   mOutGain = evt.GetInt() / OutGain.scale;
   mOutGainT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectPhaser::OnStagesText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mStagesS->SetValue((int) (mStages * Stages.scale));
}

void EffectPhaser::OnDryWetText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mDryWetS->SetValue((int) (mDryWet * DryWet.scale));
}

void EffectPhaser::OnFreqText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mFreqS->SetValue((int) (mFreq * Freq.scale));
}

void EffectPhaser::OnPhaseText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mPhaseS->SetValue((int) (mPhase * Phase.scale));
}

void EffectPhaser::OnDepthText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mDepthS->SetValue((int) (mDepth * Depth.scale));
}

void EffectPhaser::OnFeedbackText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mFeedbackS->SetValue((int) (mFeedback * Feedback.scale));
}

void EffectPhaser::OnGainText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mOutGainS->SetValue((int) (mOutGain * OutGain.scale));
}
