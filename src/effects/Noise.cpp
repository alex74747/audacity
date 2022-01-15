/**********************************************************************

  Audacity: A Digital Audio Editor

  Noise.cpp

  Dominic Mazzoni

*******************************************************************//**

\class EffectNoise
\brief An effect to add white noise.

*//*******************************************************************/


#include "Noise.h"
#include "LoadEffects.h"

#include <math.h>

#include "Prefs.h"
#include "../ShuttleGui.h"
#include "../widgets/NumericTextCtrl.h"

enum kTypes
{
   kWhite,
   kPink,
   kBrownian,
   nTypes
};

static const EnumValueSymbol kTypeStrings[nTypes] =
{
   // These are acceptable dual purpose internal/visible names
   /* i18n-hint: not a color, but "white noise" having a uniform spectrum  */
   { XC("White", "noise") },
   /* i18n-hint: not a color, but "pink noise" having a spectrum with more power
    in low frequencies */
   { XC("Pink", "noise") },
   /* i18n-hint: a kind of noise spectrum also known as "red" or "brown" */
   { XC("Brownian", "noise") }
};

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name    Type     Key               Def      Min   Max            Scale
static auto Type = EnumParameter(
                           L"Type",       kWhite,  0,    nTypes - 1, 1, kTypeStrings, nTypes  );
static auto Amp = Parameter<double>(
                           L"Amplitude",  0.8,     0.0,  1.0,           1  );

//
// EffectNoise
//

const ComponentInterfaceSymbol EffectNoise::Symbol
{ XO("Noise") };

namespace{ BuiltinEffectsModule::Registration< EffectNoise > reg; }

EffectNoise::EffectNoise()
   : mParameters{
      mType, Type,
      mAmp, Amp
   }
{
   mType = Type.def;
   mAmp = Amp.def;

   SetLinearEffectFlag(true);

   y = z = buf0 = buf1 = buf2 = buf3 = buf4 = buf5 = buf6 = 0;
}

EffectNoise::~EffectNoise()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectNoise::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectNoise::GetDescription()
{
   return XO("Generates one of three different types of noise");
}

ManualPageID EffectNoise::ManualPage()
{
   return L"Noise";
}

// EffectDefinitionInterface implementation

EffectType EffectNoise::GetType()
{
   return EffectTypeGenerate;
}

// EffectProcessor implementation

unsigned EffectNoise::GetAudioOutCount()
{
   return 1;
}

size_t EffectNoise::ProcessBlock(float **WXUNUSED(inbuf), float **outbuf, size_t size)
{
   float *buffer = outbuf[0];

   float white;
   float amplitude;
   float div = ((float) RAND_MAX) / 2.0f;

   switch (mType)
   {
   default:
   case kWhite: // white
       for (decltype(size) i = 0; i < size; i++)
       {
          buffer[i] = mAmp * ((rand() / div) - 1.0f);
       }
       break;

   case kPink: // pink
      // based on Paul Kellet's "instrumentation grade" algorithm.

      // 0.129f is an experimental normalization factor.
      amplitude = mAmp * 0.129f;
      for (decltype(size) i = 0; i < size; i++)
      {
         white = (rand() / div) - 1.0f;
         buf0 = 0.99886f * buf0 + 0.0555179f * white;
         buf1 = 0.99332f * buf1 + 0.0750759f * white;
         buf2 = 0.96900f * buf2 + 0.1538520f * white;
         buf3 = 0.86650f * buf3 + 0.3104856f * white;
         buf4 = 0.55000f * buf4 + 0.5329522f * white;
         buf5 = -0.7616f * buf5 - 0.0168980f * white;
         buffer[i] = amplitude *
            (buf0 + buf1 + buf2 + buf3 + buf4 + buf5 + buf6 + white * 0.5362);
         buf6 = white * 0.115926;
      }
      break;

   case kBrownian: // Brownian
      //float leakage=0.997; // experimental value at 44.1kHz
      //double scaling = 0.05; // experimental value at 44.1kHz
      // min and max protect against instability at extreme sample rates.
      float leakage = ((mSampleRate - 144.0) / mSampleRate < 0.9999)
         ? (mSampleRate - 144.0) / mSampleRate
         : 0.9999f;

      float scaling = (9.0 / sqrt(mSampleRate) > 0.01)
         ? 9.0 / sqrt(mSampleRate)
         : 0.01f;
 
      for (decltype(size) i = 0; i < size; i++)
      {
         white = (rand() / div) - 1.0f;
         z = leakage * y + white * scaling;
         y = fabs(z) > 1.0
            ? leakage * y - white * scaling
            : z;
         buffer[i] = mAmp * y;
      }
      break;
   }

   return size;
}

// Effect implementation

bool EffectNoise::Startup()
{
   wxString base = L"/Effects/Noise/";

   // Migrate settings from 2.1.0 or before

   // Already migrated, so bail
   if (gPrefs->Exists(base + L"Migrated"))
   {
      return true;
   }

   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
      gPrefs->Read(base + L"Type", &mType, 0L);
      gPrefs->Read(base + L"Amplitude", &mAmp, 0.8f);

      SaveUserPreset(GetCurrentSettingsGroup());

      // Do not migrate again
      gPrefs->Write(base + L"Migrated", true);
      gPrefs->Flush();
   }

   return true;
}

void EffectNoise::PopulateOrExchange(ShuttleGui & S)
{
   wxASSERT(nTypes == WXSIZEOF(kTypeStrings));

   S.StartMultiColumn(2, wxCENTER);
   {
      S
         .Target( mType )
         .AddChoice(XXO("&Noise type:"), Msgids(kTypeStrings, nTypes));

      S
         .Target( mAmp,
            NumValidatorStyle::NO_TRAILING_ZEROES, 6, Amp.min, Amp.max )
         .AddTextBox(XXO("&Amplitude (0-1):"), L"", 12);

      S
         .AddPrompt(XXO("&Duration:"));

      S
         .Text(XO("Duration"))
         .Position(wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL)
         .Target( DurationTarget() )
         .AddNumericTextCtrl( NumericConverter::TIME,
            GetDurationFormat(),
            GetDuration(),
            mProjectRate,
            NumericTextCtrl::Options{}
               .AutoPos(true));
   }
   S.EndMultiColumn();
}

