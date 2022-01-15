/**********************************************************************

  Audacity: A Digital Audio Editor

  Echo.cpp

  Dominic Mazzoni
  Vaughan Johnson (dialog)

*******************************************************************//**

\class EffectEcho
\brief An Effect that causes an echo, variable delay and volume.

*//****************************************************************//**

\class EchoDialog
\brief EchoDialog used with EffectEcho

*//*******************************************************************/


#include "Echo.h"
#include "LoadEffects.h"

#include <float.h>

#include "../ShuttleGui.h"
#include "../widgets/AudacityMessageBox.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name    Type     Key            Def   Min      Max      Scale
static auto Delay = Parameter<float>(
                           L"Delay",   1.0f, 0.001f,  FLT_MAX, 1.0f );
static auto Decay = Parameter<float>(
                           L"Decay",   0.5f, 0.0f,    FLT_MAX, 1.0f );

const ComponentInterfaceSymbol EffectEcho::Symbol
{ XO("Echo") };

namespace{ BuiltinEffectsModule::Registration< EffectEcho > reg; }

EffectEcho::EffectEcho()
   :mParameters {
      delay, Delay,
      decay, Decay,
   }
{
   Parameters().Reset();
   SetLinearEffectFlag(true);
}

EffectEcho::~EffectEcho()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectEcho::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectEcho::GetDescription()
{
   return XO("Repeats the selected audio again and again");
}

ManualPageID EffectEcho::ManualPage()
{
   return L"Echo";
}

// EffectDefinitionInterface implementation

EffectType EffectEcho::GetType()
{
   return EffectTypeProcess;
}

// EffectProcessor implementation

unsigned EffectEcho::GetAudioInCount()
{
   return 1;
}

unsigned EffectEcho::GetAudioOutCount()
{
   return 1;
}

bool EffectEcho::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames WXUNUSED(chanMap))
{
   if (delay == 0.0)
   {
      return false;
   }

   histPos = 0;
   auto requestedHistLen = (sampleCount) (mSampleRate * delay);

   // Guard against extreme delay values input by the user
   try {
      // Guard against huge delay values from the user.
      // Don't violate the assertion in as_size_t
      if (requestedHistLen !=
            (histLen = static_cast<size_t>(requestedHistLen.as_long_long())))
         throw std::bad_alloc{};
      history.reinit(histLen, true);
   }
   catch ( const std::bad_alloc& ) {
      Effect::MessageBox( XO("Requested value exceeds memory capacity.") );
      return false;
   }

   return history != NULL;
}

bool EffectEcho::ProcessFinalize()
{
   history.reset();
   return true;
}

size_t EffectEcho::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   float *ibuf = inBlock[0];
   float *obuf = outBlock[0];

   for (decltype(blockLen) i = 0; i < blockLen; i++, histPos++)
   {
      if (histPos == histLen)
      {
         histPos = 0;
      }
      history[histPos] = obuf[i] = ibuf[i] + history[histPos] * decay;
   }

   return blockLen;
}

void EffectEcho::PopulateOrExchange(ShuttleGui & S)
{
   S.AddSpace(0, 5);

   S.StartMultiColumn(2, wxALIGN_CENTER);
   {
      S
         .Target( delay,
            NumValidatorStyle::NO_TRAILING_ZEROES, 3,
            Delay.min, Delay.max )
         .AddTextBox(XXO("&Delay time (seconds):"), L"", 10);

      S
         .Target( decay,
            NumValidatorStyle::NO_TRAILING_ZEROES, 3,
            Decay.min, Decay.max)
         .AddTextBox(XXO("D&ecay factor:"), L"", 10);
   }
   S.EndMultiColumn();
}
