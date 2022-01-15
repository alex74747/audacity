/**********************************************************************

  Audacity: A Digital Audio Editor

  Repeat.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EFFECT_REPEAT__
#define __AUDACITY_EFFECT_REPEAT__

#include "Effect.h"
#include "../ShuttleAutomation.h"

class wxTextCtrl;
class ShuttleGui;

class wxStaticText;

class EffectRepeat final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectRepeat();
   virtual ~EffectRepeat();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   ManualPageID ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // Effect implementation

   bool Process() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

private:
   // EffectRepeat implementation

   void OnRepeatTextChange(wxCommandEvent & evt);
   void DisplayNewTime();

private:
   int repeatCount;

   wxTextCtrl   *mRepeatCount;
   wxStaticText *mCurrentTime;
   wxStaticText *mTotalTime;

   CapturedParameters mParameters;
   CapturedParameters &Parameters() override { return mParameters; }
   DECLARE_EVENT_TABLE()
};

#endif

