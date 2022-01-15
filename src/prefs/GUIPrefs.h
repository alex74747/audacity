/**********************************************************************

  Audacity: A Digital Audio Editor

  GUIPrefs.h

  Brian Gunlogson
  Joshua Haberman
  James Crook

**********************************************************************/

#ifndef __AUDACITY_GUI_PREFS__
#define __AUDACITY_GUI_PREFS__

#include <wx/defs.h>

#include "PrefsPanel.h"

class BoolSetting;
class ChoiceSetting;
class ShuttleGui;

#define GUI_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("GUI") }

class AUDACITY_DLL_API GUIPrefs final : public PrefsPanel
{
 public:
   GUIPrefs(wxWindow * parent, wxWindowID winid);
   ~GUIPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   ManualPageID HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;

   static void GetRangeChoices(
      TranslatableStrings *pChoices,
      Identifiers *pCodes,
      int *pDefaultRangeIndex = nullptr );

 private:
   void Populate();

   Identifiers mLangCodes;
   TranslatableStrings mLangNames;

   Identifiers mRangeCodes;
   TranslatableStrings mRangeChoices;
   int mDefaultRangeIndex;
};

AUDACITY_DLL_API
int ShowClippingPrefsID();
AUDACITY_DLL_API
int ShowTrackNameInWaveformPrefsID();

extern AUDACITY_DLL_API BoolSetting
     GUIBeepOnCompletion
   , GUIRetainLabels
   , GUIRtlWorkaround
   , GUIShowExtraMenus
   , GUIShowMac
   , GUIShowSplashScreen
;

extern AUDACITY_DLL_API ChoiceSetting
     GUIManualLocation
;

extern AUDACITY_DLL_API BoolSetting
     QuickPlayScrubbingEnabled
   , QuickPlayToolTips
;

// Right to left languages fail in many wx3 dialogs with missing buttons.
// The workaround is to use LTR in those dialogs.
void RTL_WORKAROUND( wxWindow *pWnd );

#endif
