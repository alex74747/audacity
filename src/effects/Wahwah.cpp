/**********************************************************************

  Audacity: A Digital Audio Editor

  Wahwah.cpp

  Effect programming:
  Nasca Octavian Paul (Paul Nasca)

  UI programming:
  Dominic Mazzoni (with the help of wxDesigner)
  Vaughan Johnson (Preview)

*******************************************************************//**

\class EffectWahwah
\brief An Effect that adds a 'spectral glide'.

*//*******************************************************************/


#include "Wahwah.h"
#include "LoadEffects.h"

#include <math.h>

#include <wx/slider.h>

#include "../Shuttle.h"
#include "../ShuttleGui.h"
#include "../widgets/valnum.h"

enum
{
   ID_Freq = 10000,
   ID_Phase,
   ID_Depth,
   ID_Res,
   ID_FreqOfs,
   ID_OutGain
};

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key               Def      Min      Max      Scale
static auto Freq = Parameter<double>(
                           L"Freq",       1.5,     0.1,     4.0,     10  );
static auto Phase = Parameter<double>(
                           L"Phase",      0.0,     0.0,     360.0,   1   );
static auto Depth = Parameter<int>(
                           L"Depth",      70,      0,       100,     1   ); // scaled to 0-1 before processing
static auto Res = Parameter<double>(
                           L"Resonance",  2.5,     0.1,     10.0,    10  );
static auto FreqOfs = Parameter<int>(
                           L"Offset",     30,      0,       100,     1   ); // scaled to 0-1 before processing
static auto OutGain = Parameter<double>(
                           L"Gain",      -6.0,    -30.0,    30.0,    1   );

// How many samples are processed before recomputing the lfo value again
#define lfoskipsamples 30

//
// EffectWahwah
//

const ComponentInterfaceSymbol EffectWahwah::Symbol
{ XO("Wahwah") };

namespace{ BuiltinEffectsModule::Registration< EffectWahwah > reg; }

BEGIN_EVENT_TABLE(EffectWahwah, wxEvtHandler)
    EVT_SLIDER(ID_Freq, EffectWahwah::OnFreqSlider)
    EVT_SLIDER(ID_Phase, EffectWahwah::OnPhaseSlider)
    EVT_SLIDER(ID_Depth, EffectWahwah::OnDepthSlider)
    EVT_SLIDER(ID_Res, EffectWahwah::OnResonanceSlider)
    EVT_SLIDER(ID_FreqOfs, EffectWahwah::OnFreqOffSlider)
    EVT_SLIDER(ID_OutGain, EffectWahwah::OnGainSlider)
    EVT_TEXT(ID_Freq, EffectWahwah::OnFreqText)
    EVT_TEXT(ID_Phase, EffectWahwah::OnPhaseText)
    EVT_TEXT(ID_Depth, EffectWahwah::OnDepthText)
    EVT_TEXT(ID_Res, EffectWahwah::OnResonanceText)
    EVT_TEXT(ID_FreqOfs, EffectWahwah::OnFreqOffText)
    EVT_TEXT(ID_OutGain, EffectWahwah::OnGainText)
END_EVENT_TABLE();

EffectWahwah::EffectWahwah()
{
   mFreq = Freq.def;
   mPhase = Phase.def;
   mDepth = Depth.def;
   mRes = Res.def;
   mFreqOfs = FreqOfs.def;
   mOutGain = OutGain.def;

   SetLinearEffectFlag(true);
}

EffectWahwah::~EffectWahwah()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectWahwah::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectWahwah::GetDescription()
{
   return XO("Rapid tone quality variations, like that guitar sound so popular in the 1970's");
}

ManualPageID EffectWahwah::ManualPage()
{
   return L"Wahwah";
}

// EffectDefinitionInterface implementation

EffectType EffectWahwah::GetType()
{
   return EffectTypeProcess;
}

bool EffectWahwah::SupportsRealtime()
{
#if defined(EXPERIMENTAL_REALTIME_AUDACITY_EFFECTS)
   return true;
#else
   return false;
#endif
}

// EffectProcessor implementation

unsigned EffectWahwah::GetAudioInCount()
{
   return 1;
}

unsigned EffectWahwah::GetAudioOutCount()
{
   return 1;
}

bool EffectWahwah::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames chanMap)
{
   InstanceInit(mMaster, mSampleRate);

   if (chanMap[0] == ChannelNameFrontRight)
   {
      mMaster.phase += M_PI;
   }

   return true;
}

size_t EffectWahwah::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   return InstanceProcess(mMaster, inBlock, outBlock, blockLen);
}

bool EffectWahwah::RealtimeInitialize()
{
   SetBlockSize(512);

   mSlaves.clear();

   return true;
}

bool EffectWahwah::RealtimeAddProcessor(unsigned WXUNUSED(numChannels), float sampleRate)
{
   EffectWahwahState slave;

   InstanceInit(slave, sampleRate);

   mSlaves.push_back(slave);

   return true;
}

bool EffectWahwah::RealtimeFinalize()
{
   mSlaves.clear();

   return true;
}

size_t EffectWahwah::RealtimeProcess(int group,
                                          float **inbuf,
                                          float **outbuf,
                                          size_t numSamples)
{

   return InstanceProcess(mSlaves[group], inbuf, outbuf, numSamples);
}

bool EffectWahwah::DefineParams( ShuttleParams & S ){
   S.SHUTTLE_PARAM( mFreq, Freq );
   S.SHUTTLE_PARAM( mPhase, Phase );
   S.SHUTTLE_PARAM( mDepth, Depth );
   S.SHUTTLE_PARAM( mRes, Res );
   S.SHUTTLE_PARAM( mFreqOfs, FreqOfs );
   S.SHUTTLE_PARAM( mOutGain, OutGain );
   return true;
}

bool EffectWahwah::GetAutomationParameters(CommandParameters & parms)
{
   parms.Write(Freq.key, mFreq);
   parms.Write(Phase.key, mPhase);
   parms.Write(Depth.key, mDepth);
   parms.Write(Res.key, mRes);
   parms.Write(FreqOfs.key, mFreqOfs);
   parms.Write(OutGain.key, mOutGain);
   
   return true;
}

bool EffectWahwah::SetAutomationParameters(CommandParameters & parms)
{
   ReadAndVerifyDouble(Freq);
   ReadAndVerifyDouble(Phase);
   ReadAndVerifyInt(Depth);
   ReadAndVerifyDouble(Res);
   ReadAndVerifyInt(FreqOfs);
   ReadAndVerifyDouble(OutGain);

   mFreq = Freq;
   mPhase = Phase;
   mDepth = Depth;
   mRes = Res;
   mFreqOfs = FreqOfs;
   mOutGain = OutGain;

   return true;
}

// Effect implementation

void EffectWahwah::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(5);
   S.AddSpace(0, 5);

   S.StartMultiColumn(3, wxEXPAND);
   {
      S.SetStretchyCol(2);
   
      mFreqT =
      S
         .Id(ID_Freq)
         .Validator<FloatingPointValidator<double>>(
            5, &mFreq, NumValidatorStyle::ONE_TRAILING_ZERO, Freq.min, Freq.max)
         .AddTextBox(XXO("LFO Freq&uency (Hz):"), L"", 12);

      mFreqS =
      S
         .Id(ID_Freq)
         .Text(XO("LFO frequency in hertz"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, Freq.def * Freq.scale, Freq.max * Freq.scale, Freq.min * Freq.scale);

      mPhaseT =
      S
         .Id(ID_Phase)
         .Validator<FloatingPointValidator<double>>(
            1, &mPhase, NumValidatorStyle::DEFAULT, Phase.min, Phase.max)
         .AddTextBox(XXO("LFO Sta&rt Phase (deg.):"), L"", 12);

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
         .AddTextBox(XXO("Dept&h (%):"), L"", 12);

      mDepthS =
      S
         .Id(ID_Depth)
         .Text(XO("Depth in percent"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, Depth.def * Depth.scale, Depth.max * Depth.scale, Depth.min * Depth.scale);

      mResT =
      S
         .Id(ID_Res)
         .Validator<FloatingPointValidator<double>>(
            1, &mRes, NumValidatorStyle::DEFAULT, Res.min, Res.max)
         .AddTextBox(XXO("Reso&nance:"), L"", 12);

      mResS =
      S
         .Id(ID_Res)
         .Text(XO("Resonance"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, Res.def * Res.scale, Res.max * Res.scale, Res.min * Res.scale);

      mFreqOfsT =
      S
         .Id(ID_FreqOfs)
         .Validator<IntegerValidator<int>>(
            &mFreqOfs, NumValidatorStyle::DEFAULT, FreqOfs.min, FreqOfs.max)
         .AddTextBox(XXO("Wah Frequency Offse&t (%):"), L"", 12);

      mFreqOfsS =
      S
         .Id(ID_FreqOfs)
         .Text(XO("Wah frequency offset in percent"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .AddSlider( {}, FreqOfs.def * FreqOfs.scale, FreqOfs.max * FreqOfs.scale, FreqOfs.min * FreqOfs.scale);

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

bool EffectWahwah::TransferDataToWindow()
{
   if (!mUIParent->TransferDataToWindow())
   {
      return false;
   }

   mFreqS->SetValue((int) (mFreq * Freq.scale));
   mPhaseS->SetValue((int) (mPhase * Phase.scale));
   mDepthS->SetValue((int) (mDepth * Depth.scale));
   mResS->SetValue((int) (mRes * Res.scale));
   mFreqOfsS->SetValue((int) (mFreqOfs * FreqOfs.scale));
   mOutGainS->SetValue((int) (mOutGain * OutGain.scale));

   return true;
}

bool EffectWahwah::TransferDataFromWindow()
{
   if (!mUIParent->Validate() || !mUIParent->TransferDataFromWindow())
   {
      return false;
   }

   return true;
}

// EffectWahwah implementation

void EffectWahwah::InstanceInit(EffectWahwahState & data, float sampleRate)
{
   data.samplerate = sampleRate;
   data.lfoskip = mFreq * 2 * M_PI / sampleRate;
   data.skipcount = 0;
   data.xn1 = 0;
   data.xn2 = 0;
   data.yn1 = 0;
   data.yn2 = 0;
   data.b0 = 0;
   data.b1 = 0;
   data.b2 = 0;
   data.a0 = 0;
   data.a1 = 0;
   data.a2 = 0;

   data.depth = mDepth / 100.0;
   data.freqofs = mFreqOfs / 100.0;
   data.phase = mPhase * M_PI / 180.0;
   data.outgain = DB_TO_LINEAR(mOutGain);
}

size_t EffectWahwah::InstanceProcess(EffectWahwahState & data, float **inBlock, float **outBlock, size_t blockLen)
{
   float *ibuf = inBlock[0];
   float *obuf = outBlock[0];
   double frequency, omega, sn, cs, alpha;
   double in, out;

   data.lfoskip = mFreq * 2 * M_PI / data.samplerate;
   data.depth = mDepth / 100.0;
   data.freqofs = mFreqOfs / 100.0;

   data.phase = mPhase * M_PI / 180.0;
   data.outgain = DB_TO_LINEAR(mOutGain);

   for (decltype(blockLen) i = 0; i < blockLen; i++)
   {
      in = (double) ibuf[i];

      if ((data.skipcount++) % lfoskipsamples == 0)
      {
         frequency = (1 + cos(data.skipcount * data.lfoskip + data.phase)) / 2;
         frequency = frequency * data.depth * (1 - data.freqofs) + data.freqofs;
         frequency = exp((frequency - 1) * 6);
         omega = M_PI * frequency;
         sn = sin(omega);
         cs = cos(omega);
         alpha = sn / (2 * mRes);
         data.b0 = (1 - cs) / 2;
         data.b1 = 1 - cs;
         data.b2 = (1 - cs) / 2;
         data.a0 = 1 + alpha;
         data.a1 = -2 * cs;
         data.a2 = 1 - alpha;
      };
      out = (data.b0 * in + data.b1 * data.xn1 + data.b2 * data.xn2 - data.a1 * data.yn1 - data.a2 * data.yn2) / data.a0;
      data.xn2 = data.xn1;
      data.xn1 = in;
      data.yn2 = data.yn1;
      data.yn1 = out;
      out *= data.outgain;

      obuf[i] = (float) out;
   }

   return blockLen;
}

void EffectWahwah::OnFreqSlider(wxCommandEvent & evt)
{
   mFreq = (double) evt.GetInt() / Freq.scale;
   mFreqT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectWahwah::OnPhaseSlider(wxCommandEvent & evt)
{
   int val = ((evt.GetInt() + 5) / 10) * 10; // round to nearest multiple of 10
   val = val > Phase.max * Phase.scale ? Phase.max * Phase.scale : val;
   mPhaseS->SetValue(val);
   mPhase = (double) val / Phase.scale;
   mPhaseT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectWahwah::OnDepthSlider(wxCommandEvent & evt)
{
   mDepth = evt.GetInt() / Depth.scale;
   mDepthT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectWahwah::OnResonanceSlider(wxCommandEvent & evt)
{
   mRes = (double) evt.GetInt() / Res.scale;
   mResT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectWahwah::OnFreqOffSlider(wxCommandEvent & evt)
{
   mFreqOfs = evt.GetInt() / FreqOfs.scale;
   mFreqOfsT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectWahwah::OnGainSlider(wxCommandEvent & evt)
{
   mOutGain = evt.GetInt() / OutGain.scale;
   mOutGainT->GetValidator()->TransferToWindow();
   EnableApply(mUIParent->Validate());
}

void EffectWahwah::OnFreqText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mFreqS->SetValue((int) (mFreq * Freq.scale));
}

void EffectWahwah::OnPhaseText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mPhaseS->SetValue((int) (mPhase * Phase.scale));
}

void EffectWahwah::OnDepthText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mDepthS->SetValue((int) (mDepth * Depth.scale));
}

void EffectWahwah::OnResonanceText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mResS->SetValue((int) (mRes * Res.scale));
}

void EffectWahwah::OnFreqOffText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mFreqOfsS->SetValue((int) (mFreqOfs * FreqOfs.scale));
}

void EffectWahwah::OnGainText(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   mOutGainS->SetValue((int) (mOutGain * OutGain.scale));
}
