/**********************************************************************

  Audacity: A Digital Audio Editor

  GUIPrefs.cpp

  Brian Gunlogson
  Joshua Haberman
  Dominic Mazzoni
  James Crook


*******************************************************************//**

\class GUIPrefs
\brief A PrefsPanel for general GUI preferences.

*//*******************************************************************/


#include "GUIPrefs.h"

#include <wx/defs.h>

#include "FileNames.h"
#include "Languages.h"
#include "Theme.h"
#include "Prefs.h"
#include "../ShuttleGui.h"

#include "Decibels.h"

#include "ThemePrefs.h"
#include "AColor.h"
#include "GUISettings.h"

GUIPrefs::GUIPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: refers to Audacity's user interface settings */
:  PrefsPanel(parent, winid, XC("Interface", "GUI"))
{
   Populate();
}

GUIPrefs::~GUIPrefs()
{
}

ComponentInterfaceSymbol GUIPrefs::GetSymbol()
{
   return GUI_PREFS_PLUGIN_SYMBOL;
}

TranslatableString GUIPrefs::GetDescription()
{
   return XO("Preferences for GUI");
}

ManualPageID GUIPrefs::HelpPageName()
{
   return "Interface_Preferences";
}

void GUIPrefs::GetRangeChoices(
   TranslatableStrings *pChoices,
   Identifiers *pCodes,
   int *pDefaultRangeIndex )
{
   static const Identifiers sCodes = {
      L"36" ,
      L"48" ,
      L"60" ,
      L"72" ,
      L"84" ,
      L"96" ,
      L"120" ,
      L"145" ,
   };
   if (pCodes)
      *pCodes = sCodes;

   static const std::initializer_list<TranslatableString> sChoices = {
      XO("-36 dB (shallow range for high-amplitude editing)") ,
      XO("-48 dB (PCM range of 8 bit samples)") ,
      XO("-60 dB (PCM range of 10 bit samples)") ,
      XO("-72 dB (PCM range of 12 bit samples)") ,
      XO("-84 dB (PCM range of 14 bit samples)") ,
      XO("-96 dB (PCM range of 16 bit samples)") ,
      XO("-120 dB (approximate limit of human hearing)") ,
      XO("-145 dB (PCM range of 24 bit samples)") ,
   };

   if (pChoices)
      *pChoices = sChoices;

   if (pDefaultRangeIndex)
      *pDefaultRangeIndex = 2; // 60 == DecibelScaleCutoff.GetDefault()
}

void GUIPrefs::Populate()
{
   // First any pre-processing for constructing the GUI.
   TranslatableStrings langNames;
   Languages::GetLanguages(
      FileNames::AudacityPathList(), mLangCodes, langNames);
   mLangNames = { langNames.begin(), langNames.end() };

   TranslatableStrings rangeChoices;
   GetRangeChoices(&rangeChoices, &mRangeCodes, &mDefaultRangeIndex);
   mRangeChoices = { rangeChoices.begin(), rangeChoices.end() };

#if 0
   mLangCodes.insert( mLangCodes.end(), {
      // only for testing...
      "kg" ,
      "ep" ,
   } );

   mLangNames.insert( mLangNames.end(), {
      "Klingon" ,
      "Esperanto" ,
   } );
#endif
}

ChoiceSetting GUIManualLocation{
   L"/GUI/Help",
   {
      ByColumns,
      { XO("Local") ,  XO("From Internet") , },
      { L"Local" , L"FromInternet" , }
   },
   0 // "Local"
};

void GUIPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(XO("Display"));
   {
      S.StartMultiColumn(2);
      {

         S
            .TieChoice( XXO("&Language:"),
               {
                  LocaleLanguage,
                  { ByColumns, mLangNames, mLangCodes }
               } );

         S
            .TieChoice( XXO("Location of &Manual:"), GUIManualLocation);

         S
            .TieChoice( XXO("Th&eme:"), GUITheme());

         S
            .TieChoice( XXO("Meter dB &range:"),
               {
                  DecibelScaleCutoff.GetPath(),
                  { ByColumns, mRangeChoices, mRangeCodes },
                  mDefaultRangeIndex
               } );
      }
      S.EndMultiColumn();

   }
   S.EndStatic();

   S.StartStatic(XO("Options"));
   {
      S
         // Start wording of options with a verb, if possible.
         .TieCheckBox(XXO("Show 'How to Get &Help' at launch"),
            GUIShowSplashScreen);

      S
         .TieCheckBox(XXO("Show e&xtra menus"),
           GUIShowExtraMenus);

#ifdef EXPERIMENTAL_THEME_PREFS
      // We do not want to make this option mainstream.  It's a 
      // convenience for developers.
      S
         .TieCheckBox(XXO("Show alternative &styling (Mac vs PC)"),
            GUIShowMac);
#endif
      S
         .TieCheckBox(XXO("&Beep on completion of longer activities"),
            GUIBeepOnCompletion);
      S
         .TieCheckBox(XXO("Re&tain labels if selection snaps to a label"),
            GUIRetainLabels);
      S
         .TieCheckBox(XXO("B&lend system and Audacity theme"),
            GUIBlendThemes);
#ifndef __WXMAC__
      /* i18n-hint: RTL stands for 'Right to Left'  */
      S
         .TieCheckBox(XXO("Use mostly Left-to-Right layouts in RTL languages"),
            GUIRtlWorkaround);
#endif
#ifdef EXPERIMENTAL_CEE_NUMBERS_OPTION
      S.TieCheckBox(XXO("Never use comma as decimal point"),
                    {L"/Locale/CeeNumberFormat",
                     false});
#endif
   }
   S.EndStatic();

   S.StartStatic(XO("Timeline"));
   {
      S
         .TieCheckBox(XXO("Show Timeline Tooltips"),
            QuickPlayToolTips);

      S
         .TieCheckBox(XXO("Show Scrub Ruler"),
            QuickPlayScrubbingEnabled);
   }
   S.EndStatic();

   S.EndScroller();
}

bool GUIPrefs::Commit()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   // If language has changed, we want to change it now, not on the next reboot.
   Identifier lang = LocaleLanguage.Read();
   auto usedLang = GUISettings::SetLang(lang);
   // Bug 1523: Previously didn't check no-language (=System Language)
   if (!(lang.empty() || lang == L"System") && (lang != usedLang)) {
      // lang was not usable and is not system language.  We got overridden.
      LocaleLanguage.Write( usedLang.GET() );
      gPrefs->Flush();
   }

   // Reads preference GUITheme
   {
      wxBusyCursor busy;
      theTheme.LoadPreferredTheme();
      theTheme.DeleteUnusedThemes();
   }
   ThemePrefs::ApplyUpdatedImages();

   return true;
}

int ShowClippingPrefsID()
{
   static int value = wxNewId();
   return value;
}

int ShowTrackNameInWaveformPrefsID()
{
   static int value = wxNewId();
   return value;
}

namespace{
PrefsPanel::Registration sAttachment{ "GUI",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew GUIPrefs(parent, winid);
   }
};
}

void RTL_WORKAROUND( wxWindow *pWnd )
{
#ifndef __WXMAC__
   if ( GUIRtlWorkaround.Read() )
      pWnd->SetLayoutDirection(wxLayout_LeftToRight);
#else
   (void)pWnd;
#endif
}

BoolSetting GUIBeepOnCompletion{
   L"/GUI/BeepOnCompletion", false };
BoolSetting GUIRetainLabels{
   L"/GUI/RetainLabels",     false };
BoolSetting GUIRtlWorkaround{
   L"/GUI/RtlWorkaround",    true  };
BoolSetting GUIShowExtraMenus{
   L"/GUI/ShowExtraMenus",   false };
BoolSetting GUIShowMac{
   L"/GUI/ShowMac",          false };
BoolSetting GUIShowSplashScreen{
   L"/GUI/ShowSplashScreen", true  };

// A dB value as a positivie number
IntSetting GUIdBRange{
   L"/GUI/EnvdBRange",       60 };

StringSetting LocaleLanguage{
   L"/Locale/Language",      L"" };

BoolSetting QuickPlayScrubbingEnabled{
   L"/QuickPlay/ScrubbingEnabled",   false};
BoolSetting QuickPlayToolTips{
   L"/QuickPlay/ToolTips",   true};

