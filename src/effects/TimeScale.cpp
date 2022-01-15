/**********************************************************************

  Audacity: A Digital Audio Editor

  TimeScale.cpp

  Clayton Otey

*******************************************************************//**

\class EffectTimeScale
\brief An EffectTimeScale does high quality sliding time scaling/pitch shifting

*//*******************************************************************/



#if USE_SBSMS
#include "TimeScale.h"
#include "LoadEffects.h"

#include <math.h>

#include <wx/slider.h>

#include "../Shuttle.h"
#include "../ShuttleGui.h"
#include "../widgets/valnum.h"

enum
{
   ID_RatePercentChangeStart = 10000,
   ID_RatePercentChangeEnd,
   ID_PitchHalfStepsStart,
   ID_PitchHalfStepsEnd,
   ID_PitchPercentChangeStart,
   ID_PitchPercentChangeEnd
};

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name                Type    Key                            Def   Min      Max    Scale
static auto RatePercentStart = Parameter<double>(
                           L"RatePercentChangeStart",  0.0,  -90.0,   500,   1  );
static auto RatePercentEnd = Parameter<double>(
                           L"RatePercentChangeEnd",    0.0,  -90.0,   500,   1  );
static auto HalfStepsStart = Parameter<double>(
                           L"PitchHalfStepsStart",     0.0,  -12.0,   12.0,  1  );
static auto HalfStepsEnd = Parameter<double>(
                           L"PitchHalfStepsEnd",       0.0,  -12.0,   12.0,  1  );
static auto PitchPercentStart = Parameter<double>(
                           L"PitchPercentChangeStart", 0.0,  -50.0,   100.0, 1  );
static auto PitchPercentEnd = Parameter<double>(
                           L"PitchPercentChangeEnd",   0.0,  -50.0,   100.0, 1  );

//
// EffectTimeScale
//

const ComponentInterfaceSymbol EffectTimeScale::Symbol
{ L"Sliding Stretch", XO("Sliding Stretch") };

namespace{ BuiltinEffectsModule::Registration< EffectTimeScale > reg; }

BEGIN_EVENT_TABLE(EffectTimeScale, wxEvtHandler)
   EVT_TEXT(ID_RatePercentChangeStart, EffectTimeScale::OnText_RatePercentChangeStart)
   EVT_TEXT(ID_RatePercentChangeEnd, EffectTimeScale::OnText_RatePercentChangeEnd)
   EVT_TEXT(ID_PitchHalfStepsStart, EffectTimeScale::OnText_PitchHalfStepsStart)
   EVT_TEXT(ID_PitchHalfStepsEnd, EffectTimeScale::OnText_PitchHalfStepsEnd)
   EVT_TEXT(ID_PitchPercentChangeStart, EffectTimeScale::OnText_PitchPercentChangeStart)
   EVT_TEXT(ID_PitchPercentChangeEnd, EffectTimeScale::OnText_PitchPercentChangeEnd)
   EVT_SLIDER(ID_RatePercentChangeStart, EffectTimeScale::OnSlider_RatePercentChangeStart)
   EVT_SLIDER(ID_RatePercentChangeEnd, EffectTimeScale::OnSlider_RatePercentChangeEnd)
END_EVENT_TABLE()

EffectTimeScale::EffectTimeScale()
{
   m_RatePercentChangeStart = RatePercentStart.def;
   m_RatePercentChangeEnd = RatePercentEnd.def;
   m_PitchHalfStepsStart = HalfStepsStart.def;
   m_PitchHalfStepsEnd = HalfStepsEnd.def;
   m_PitchPercentChangeStart = PitchPercentStart.def;
   m_PitchPercentChangeEnd = PitchPercentEnd.def;

   slideTypeRate = SlideLinearOutputRate;
   slideTypePitch = SlideLinearOutputRate;
   bPreview = false;
   previewSelectedDuration = 0.0;
   
   SetLinearEffectFlag(true);
}

EffectTimeScale::~EffectTimeScale()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectTimeScale::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectTimeScale::GetDescription()
{
   return XO("Allows continuous changes to the tempo and/or pitch");
}

ManualPageID EffectTimeScale::ManualPage()
{
   return L"Sliding_Stretch";
}

// EffectDefinitionInterface implementation

EffectType EffectTimeScale::GetType()
{
   return EffectTypeProcess;
}

// EffectProcessor implementation
bool EffectTimeScale::DefineParams( ShuttleParams & S ){
   S.SHUTTLE_PARAM( m_RatePercentChangeStart,  RatePercentStart );
   S.SHUTTLE_PARAM( m_RatePercentChangeEnd,    RatePercentEnd );
   S.SHUTTLE_PARAM( m_PitchHalfStepsStart,     HalfStepsStart );
   S.SHUTTLE_PARAM( m_PitchHalfStepsEnd,       HalfStepsEnd );
   S.SHUTTLE_PARAM( m_PitchPercentChangeStart, PitchPercentStart );
   S.SHUTTLE_PARAM( m_PitchPercentChangeEnd,   PitchPercentEnd );
   return true;
}

bool EffectTimeScale::GetAutomationParameters(CommandParameters & parms)
{
   parms.Write(RatePercentStart.key, m_RatePercentChangeStart);
   parms.Write(RatePercentEnd.key, m_RatePercentChangeEnd);
   parms.Write(HalfStepsStart.key, m_PitchHalfStepsStart);
   parms.Write(HalfStepsEnd.key, m_PitchHalfStepsEnd);
   parms.Write(PitchPercentStart.key, m_PitchPercentChangeStart);
   parms.Write(PitchPercentEnd.key, m_PitchPercentChangeEnd);

   return true;
}

bool EffectTimeScale::SetAutomationParameters(CommandParameters & parms)
{
   ReadAndVerifyDouble(RatePercentStart);
   ReadAndVerifyDouble(RatePercentEnd);
   ReadAndVerifyDouble(HalfStepsStart);
   ReadAndVerifyDouble(HalfStepsEnd);
   ReadAndVerifyDouble(PitchPercentStart);
   ReadAndVerifyDouble(PitchPercentEnd);

   m_RatePercentChangeStart = RatePercentStart;
   m_RatePercentChangeEnd = RatePercentEnd;
   m_PitchHalfStepsStart = HalfStepsStart;
   m_PitchHalfStepsEnd = HalfStepsEnd;
   m_PitchPercentChangeStart = PitchPercentStart;
   m_PitchPercentChangeEnd = PitchPercentEnd;
   
   return true;
}

// Effect implementation

bool EffectTimeScale::Init()
{
   return true;
}

double EffectTimeScale::CalcPreviewInputLength(double previewLength)
{
   double inputLength = Effect::GetDuration();
   if(inputLength == 0.0) {
      return 0.0;
   } else {
      double rateStart1 = PercentChangeToRatio(m_RatePercentChangeStart);
      double rateEnd1 = PercentChangeToRatio(m_RatePercentChangeEnd);
      double tOut = previewLength/inputLength;
      double t = EffectSBSMS::getInvertedStretchedTime(rateStart1,rateEnd1,slideTypeRate,tOut);
      return t * inputLength;
   }
}

void EffectTimeScale::Preview(bool dryOnly)
{
   previewSelectedDuration = Effect::GetDuration();
   auto cleanup = valueRestorer( bPreview, true );
   Effect::Preview(dryOnly);
}

bool EffectTimeScale::Process()
{
   double pitchStart1 = PercentChangeToRatio(m_PitchPercentChangeStart);
   double pitchEnd1 = PercentChangeToRatio(m_PitchPercentChangeEnd);
   double rateStart1 = PercentChangeToRatio(m_RatePercentChangeStart);
   double rateEnd1 = PercentChangeToRatio(m_RatePercentChangeEnd);
  
   if(bPreview) {
      double t = (mT1-mT0) / previewSelectedDuration;
      rateEnd1 = EffectSBSMS::getRate(rateStart1,rateEnd1,slideTypeRate,t);
      pitchEnd1 = EffectSBSMS::getRate(pitchStart1,pitchEnd1,slideTypePitch,t);
   }
   
   EffectSBSMS::setParameters(rateStart1,rateEnd1,pitchStart1,pitchEnd1,slideTypeRate,slideTypePitch,false,false,false);
   return EffectSBSMS::Process();
}

void EffectTimeScale::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(5);
   S.AddSpace(0, 5);

   S.StartMultiColumn(2, wxALIGN_CENTER);
   {
      // Rate Start
      S.StartStatic(XO("Initial Tempo Change (%)"));
      {
         S.StartMultiColumn(1, wxCENTER);
         {
            m_pTextCtrl_RatePercentChangeStart =
            S
               .Id(ID_RatePercentChangeStart)
               .Validator<FloatingPointValidator<double>>(
                  3, &m_RatePercentChangeStart,
                  NumValidatorStyle::NO_TRAILING_ZEROES,
                  RatePercentStart.min, RatePercentStart.max )
               .AddTextBox( {}, L"", 12);
         }
         S.EndMultiColumn();
         S.StartHorizontalLay(wxEXPAND, 0);
         {
            m_pSlider_RatePercentChangeStart =
            S
               .Id(ID_RatePercentChangeStart)
               .Style(wxSL_HORIZONTAL)
               .AddSlider( {}, RatePercentStart.def, RatePercentStart.max, RatePercentStart.min);
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S.StartStatic(XO("Final Tempo Change (%)"));
      {
         S.StartMultiColumn(1, wxCENTER);
         {
            m_pTextCtrl_RatePercentChangeEnd =
            S
               .Id(ID_RatePercentChangeEnd)
               .Validator<FloatingPointValidator<double>>(
                  3, &m_RatePercentChangeEnd,
                  NumValidatorStyle::NO_TRAILING_ZEROES,
                  RatePercentEnd.min, RatePercentEnd.max )
               .AddTextBox( {}, L"", 12);
         }
         S.EndMultiColumn();
         S.StartHorizontalLay(wxEXPAND, 0);
         {
            m_pSlider_RatePercentChangeEnd =
            S
               .Id(ID_RatePercentChangeEnd)
               .Style(wxSL_HORIZONTAL)
               .AddSlider( {}, RatePercentEnd.def, RatePercentEnd.max, RatePercentEnd.min);
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      // Pitch Start
      S.StartStatic(XO("Initial Pitch Shift"));
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            m_pTextCtrl_PitchHalfStepsStart =
            S
               .Id(ID_PitchHalfStepsStart)
               .Validator<FloatingPointValidator<double>>(
                  3, &m_PitchHalfStepsStart,
                  NumValidatorStyle::NO_TRAILING_ZEROES,
                  HalfStepsStart.min, HalfStepsStart.max )
               .AddTextBox(XXO("(&semitones) [-12 to 12]:"), L"", 12);


            m_pTextCtrl_PitchPercentChangeStart =
            S
               .Id(ID_PitchPercentChangeStart)
               .Validator<FloatingPointValidator<double>>(
                  3, &m_PitchPercentChangeStart,
                  NumValidatorStyle::NO_TRAILING_ZEROES,
                  PitchPercentStart.min, PitchPercentStart.max )
               .AddTextBox(XXO("(%) [-50 to 100]:"), L"", 12);
         }
         S.EndMultiColumn();
      }
      S.EndStatic();

      // Pitch End
      S.StartStatic(XO("Final Pitch Shift"));
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            m_pTextCtrl_PitchHalfStepsEnd =
            S
               .Id(ID_PitchHalfStepsEnd)
               .Validator<FloatingPointValidator<double>>(
                  3, &m_PitchHalfStepsEnd,
                  NumValidatorStyle::NO_TRAILING_ZEROES,
                  HalfStepsEnd.min, HalfStepsEnd.max )
               .AddTextBox(XXO("(s&emitones) [-12 to 12]:"), L"", 12);

            m_pTextCtrl_PitchPercentChangeEnd =
            S
               .Id(ID_PitchPercentChangeEnd)
               .Validator<FloatingPointValidator<double>>(
                  3, &m_PitchPercentChangeEnd,
                  NumValidatorStyle::NO_TRAILING_ZEROES,
                  PitchPercentStart.min, PitchPercentStart.max)
               .AddTextBox(XXO("(%) [-50 to 100]:"), L"", 12);
         }
         S.EndMultiColumn();
      }
      S.EndStatic();
   }
   S.EndMultiColumn();

   return;
}

bool EffectTimeScale::TransferDataToWindow()
{
   if (!mUIParent->TransferDataToWindow())
   {
      return false;
   }

   Update_Slider_RatePercentChangeStart();
   Update_Slider_RatePercentChangeEnd();

   return true;
}

bool EffectTimeScale::TransferDataFromWindow()
{
   if (!mUIParent->Validate() || !mUIParent->TransferDataFromWindow())
   {
      return false;
   }

   return true;
}

inline double EffectTimeScale::PercentChangeToRatio(double percentChange)
{
   return 1.0 + percentChange / 100.0;
}

inline double EffectTimeScale::HalfStepsToPercentChange(double halfSteps)
{
   return 100.0 * (pow(2.0,halfSteps/12.0) - 1.0);
}

inline double EffectTimeScale::PercentChangeToHalfSteps(double percentChange)
{
   return 12.0 * log2(PercentChangeToRatio(percentChange));
}

void EffectTimeScale::Update_Text_RatePercentChangeStart()
{
   m_pTextCtrl_RatePercentChangeStart->GetValidator()->TransferToWindow();
}

void EffectTimeScale::Update_Text_RatePercentChangeEnd()
{
   m_pTextCtrl_RatePercentChangeEnd->GetValidator()->TransferToWindow();
}

void EffectTimeScale::Update_Slider_RatePercentChangeStart()
{
   m_pSlider_RatePercentChangeStart->SetValue((int)(m_RatePercentChangeStart + 0.5));
}

void EffectTimeScale::Update_Slider_RatePercentChangeEnd()
{
   m_pSlider_RatePercentChangeEnd->SetValue((int)(m_RatePercentChangeEnd + 0.5));
}

void EffectTimeScale::Update_Text_PitchHalfStepsStart()
{
   m_pTextCtrl_PitchHalfStepsStart->GetValidator()->TransferToWindow();
}

void EffectTimeScale::Update_Text_PitchHalfStepsEnd()
{
   m_pTextCtrl_PitchHalfStepsEnd->GetValidator()->TransferToWindow();
}

void EffectTimeScale::Update_Text_PitchPercentChangeStart()
{
   m_pTextCtrl_PitchPercentChangeStart->GetValidator()->TransferToWindow();
}

void EffectTimeScale::Update_Text_PitchPercentChangeEnd()
{
   m_pTextCtrl_PitchPercentChangeEnd->GetValidator()->TransferToWindow();
}

void EffectTimeScale::OnText_RatePercentChangeStart(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   Update_Slider_RatePercentChangeStart();
}

void EffectTimeScale::OnText_RatePercentChangeEnd(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   Update_Slider_RatePercentChangeEnd();
}

void EffectTimeScale::OnSlider_RatePercentChangeStart(wxCommandEvent & evt)
{
   m_RatePercentChangeStart = (double) evt.GetInt();

   Update_Text_RatePercentChangeStart();
}

void EffectTimeScale::OnSlider_RatePercentChangeEnd(wxCommandEvent & evt)
{
   m_RatePercentChangeEnd = (double) evt.GetInt();

   Update_Text_RatePercentChangeEnd();
}

void EffectTimeScale::OnText_PitchHalfStepsStart(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   m_PitchPercentChangeStart = HalfStepsToPercentChange(m_PitchHalfStepsStart);
   Update_Text_PitchPercentChangeStart();
}

void EffectTimeScale::OnText_PitchHalfStepsEnd(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   m_PitchPercentChangeEnd = HalfStepsToPercentChange(m_PitchHalfStepsEnd);
   Update_Text_PitchPercentChangeEnd();
}

void EffectTimeScale::OnText_PitchPercentChangeStart(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   m_PitchHalfStepsStart = PercentChangeToHalfSteps(m_PitchPercentChangeStart);
   Update_Text_PitchHalfStepsStart();
}

void EffectTimeScale::OnText_PitchPercentChangeEnd(wxCommandEvent & WXUNUSED(evt))
{
   if (!EnableApply(mUIParent->TransferDataFromWindow()))
   {
      return;
   }

   m_PitchHalfStepsEnd = PercentChangeToHalfSteps(m_PitchPercentChangeEnd);
   Update_Text_PitchHalfStepsEnd();
}

#endif
