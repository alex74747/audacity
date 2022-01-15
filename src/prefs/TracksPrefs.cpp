/**********************************************************************

  Audacity: A Digital Audio Editor

  TracksPrefs.cpp

  Brian Gunlogson
  Joshua Haberman
  Dominic Mazzoni
  James Crook


*******************************************************************//**

\class TracksPrefs
\brief A PrefsPanel for track display and behavior properties.

*//*******************************************************************/


#include "TracksPrefs.h"
#include "MemoryX.h"

//#include <algorithm>
//#include <wx/defs.h>

#include "Prefs.h"
#include "../ShuttleGui.h"
#include "../WaveTrack.h"

int TracksPrefs::iPreferencePinned = -1;

namespace {
   const wxChar *PinnedHeadPreferenceKey()
   {
      return L"/AudioIO/PinnedHead";
   }

   bool PinnedHeadPreferenceDefault()
   {
      return false;
   }
   
   const wxChar *PinnedHeadPositionPreferenceKey()
   {
      return L"/AudioIO/PinnedHeadPosition";
   }

   double PinnedHeadPositionPreferenceDefault()
   {
      return 0.5;
   }
}


namespace {
   const auto waveformScaleKey = L"/GUI/DefaultWaveformScaleChoice";
   const auto dbValueString = L"dB";
}

static EnumSetting< WaveformSettings::ScaleTypeValues > waveformScaleSetting{
   waveformScaleKey,
   {
      { XO("Linear") },
      { dbValueString, XO("Logarithmic (dB)") },
   },

   0, // linear
   
   {
      WaveformSettings::stLinear,
      WaveformSettings::stLogarithmic,
   }
};

//////////
// There is a complicated migration history here!
namespace {
   const auto key0 = L"/GUI/DefaultViewMode";
   const auto key1 = L"/GUI/DefaultViewModeNew";
   const auto key2 = L"/GUI/DefaultViewModeChoice";
   const auto key3 = L"/GUI/DefaultViewModeChoiceNew";

   const wxString obsoleteValue{ L"WaveformDB" };
};

class TracksViewModeEnumSetting
   : public EnumSetting< WaveTrackViewConstants::Display > {
public:
   using EnumSetting< WaveTrackViewConstants::Display >::EnumSetting;

   void Migrate( wxString &value ) override
   {
      // Special logic for this preference which was three times migrated!

      // PRL:  Bugs 1043, 1044
      // 2.1.1 writes a NEW key for this preference, which got NEW values,
      // to avoid confusing version 2.1.0 if it reads the preference file afterwards.
      // Prefer the NEW preference key if it is present

      static const EnumValueSymbol waveformSymbol{ XO("Waveform") };
      static const EnumValueSymbol spectrumSymbol{ XO("Spectrogram") };

      WaveTrackViewConstants::Display viewMode;
      int oldMode;
      wxString newValue;
      auto stringValue =
         []( WaveTrackViewConstants::Display display ){
         switch ( display ) {
            case WaveTrackViewConstants::Spectrum:
               return spectrumSymbol.Internal();
            case WaveTrackViewConstants::obsoleteWaveformDBDisplay:
               return obsoleteValue;
            default:
               return waveformSymbol.Internal();
         }
      };

      if ( gPrefs->Read(key0, // The very old key
         &oldMode,
         (int)(WaveTrackViewConstants::Waveform) ) ) {
         viewMode = WaveTrackViewConstants::ConvertLegacyDisplayValue(oldMode);
         newValue = stringValue( viewMode );
      }
      else if ( gPrefs->Read(key1,
         &oldMode,
         (int)(WaveTrackViewConstants::Waveform) ) ) {
         viewMode = static_cast<WaveTrackViewConstants::Display>( oldMode );
         newValue = stringValue( viewMode );
      }
      else
         gPrefs->Read( key2, &newValue );

      if ( !gPrefs->Read( key3, &value ) ) {
         if (newValue == obsoleteValue) {
            newValue = waveformSymbol.Internal();
            gPrefs->Write(waveformScaleKey, dbValueString);
         }

         Write( value = newValue );
         gPrefs->Flush();
         return;
      }
   }
};

static TracksViewModeEnumSetting viewModeSetting()
{
   // Do a delayed computation, so that registration of sub-view types completes
   // first
   const auto &types = WaveTrackSubViewType::All();
   EnumValueSymbols symbols;
   for ( const auto &symbol : types )
      symbols.emplace_back( symbol.name.Internal(), symbol.name.Stripped() );
   auto ids = transform_container< std::vector< WaveTrackSubViewType::Display > >(
      types, std::mem_fn( &WaveTrackSubViewType::id ) );

   // Special entry for multi
   symbols.push_back( WaveTrackViewConstants::MultiViewSymbol );
   ids.push_back( WaveTrackViewConstants::MultiView );

   return {
      key3,
      symbols,
      0, // Waveform
      ids
   };
}

WaveTrackViewConstants::Display TracksPrefs::ViewModeChoice()
{
   return viewModeSetting().ReadEnum();
}

WaveformSettings::ScaleTypeValues TracksPrefs::WaveformScaleChoice()
{
   return waveformScaleSetting.ReadEnum();
}

//////////
static EnumSetting< WaveTrackViewConstants::SampleDisplay >
sampleDisplaySetting{
   L"/GUI/SampleViewChoice",
   {
      { L"ConnectDots", XO("Connect dots") },
      { L"StemPlot", XO("Stem plot") }
   },
   1, // StemPlot

   // for migrating old preferences:
   {
      WaveTrackViewConstants::LinearInterpolate,
      WaveTrackViewConstants::StemPlot
   },
   L"/GUI/SampleView"
};

WaveTrackViewConstants::SampleDisplay TracksPrefs::SampleViewChoice()
{
   return sampleDisplaySetting.ReadEnum();
}

//////////
static const std::initializer_list<EnumValueSymbol> choicesZoom{
   { L"FitToWidth", XO("Fit to Width") },
   { L"ZoomToSelection", XO("Zoom to Selection") },
   { L"ZoomDefault", XO("Zoom Default") },
   { XO("Minutes") },
   { XO("Seconds") },
   { L"FifthsOfSeconds", XO("5ths of Seconds") },
   { L"TenthsOfSeconds", XO("10ths of Seconds") },
   { L"TwentiethsOfSeconds", XO("20ths of Seconds") },
   { L"FiftiethsOfSeconds", XO("50ths of Seconds") },
   { L"HundredthsOfSeconds", XO("100ths of Seconds") },
   { L"FiveHundredthsOfSeconds", XO("500ths of Seconds") },
   { XO("MilliSeconds") },
   { XO("Samples") },
   { L"FourPixelsPerSample", XO("4 Pixels per Sample") },
   { L"MaxZoom", XO("Max Zoom") },
};
static auto enumChoicesZoom = {
   WaveTrackViewConstants::kZoomToFit,
   WaveTrackViewConstants::kZoomToSelection,
   WaveTrackViewConstants::kZoomDefault,
   WaveTrackViewConstants::kZoomMinutes,
   WaveTrackViewConstants::kZoomSeconds,
   WaveTrackViewConstants::kZoom5ths,
   WaveTrackViewConstants::kZoom10ths,
   WaveTrackViewConstants::kZoom20ths,
   WaveTrackViewConstants::kZoom50ths,
   WaveTrackViewConstants::kZoom100ths,
   WaveTrackViewConstants::kZoom500ths,
   WaveTrackViewConstants::kZoomMilliSeconds,
   WaveTrackViewConstants::kZoomSamples,
   WaveTrackViewConstants::kZoom4To1,
   WaveTrackViewConstants::kMaxZoom,
};

static EnumSetting< WaveTrackViewConstants::ZoomPresets > zoom1Setting{
   L"/GUI/ZoomPreset1Choice",
   choicesZoom,
   2, // kZoomDefault

   // for migrating old preferences:
   enumChoicesZoom,
   L"/GUI/ZoomPreset1"
};

static EnumSetting< WaveTrackViewConstants::ZoomPresets > zoom2Setting{
   L"/GUI/ZoomPreset2Choice",
   choicesZoom,
   13, // kZoom4To1

   // for migrating old preferences:
   enumChoicesZoom,
   L"/GUI/ZoomPreset2"
};

WaveTrackViewConstants::ZoomPresets TracksPrefs::Zoom1Choice()
{
   return zoom1Setting.ReadEnum();
}

WaveTrackViewConstants::ZoomPresets TracksPrefs::Zoom2Choice()
{
   return zoom2Setting.ReadEnum();
}

//////////
TracksPrefs::TracksPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: "Tracks" include audio recordings but also other collections of
 * data associated with a time line, such as sequences of labels, and musical
 * notes */
:  PrefsPanel(parent, winid, XO("Tracks"))
{
}

TracksPrefs::~TracksPrefs()
{
}

ComponentInterfaceSymbol TracksPrefs::GetSymbol()
{
   return TRACKS_PREFS_PLUGIN_SYMBOL;
}

TranslatableString TracksPrefs::GetDescription()
{
   return XO("Preferences for Tracks");
}

ManualPageID TracksPrefs::HelpPageName()
{
   return "Tracks_Preferences";
}

void TracksPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S
      .StartStatic(XO("Display"));
   {
      S
         .TieCheckBox(XXO("Auto-&fit track height"),
            {L"/GUI/TracksFitVerticallyZoomed", false});

      S
         .TieCheckBox(XXO("Sho&w track name as overlay"),
            {L"/GUI/ShowTrackNameInWaveform", false});

#ifdef EXPERIMENTAL_HALF_WAVE
      S
         .TieCheckBox(XXO("Use &half-wave display when collapsed"),
            {L"/GUI/CollapseToHalfWave", false});
#endif

#ifdef SHOW_PINNED_UNPINNED_IN_PREFS
      S
         .TieCheckBox(XXO("&Pinned Recording/Playback head"),
            {PinnedHeadPreferenceKey(), PinnedHeadPreferenceDefault()});
#endif

      S
         .TieCheckBox(XXO("A&uto-scroll if head unpinned"),
            {L"/GUI/AutoScroll", true});

      S.AddSpace(10);

      S.StartMultiColumn(2);
      {
#define SHOW_PINNED_POSITION_IN_PREFS
#ifdef SHOW_PINNED_POSITION_IN_PREFS
         S
            .TieNumericTextBox(
               XXO("Pinned &head position"),
               {PinnedHeadPositionPreferenceKey(),
                PinnedHeadPositionPreferenceDefault()},
               30 );
#endif

         S
            .TieChoice( XXO("Default &view mode:"), viewModeSetting() );

         S
            .TieChoice( XXO("Default Waveform scale:"), waveformScaleSetting );

         S
            .TieChoice( XXO("Display &samples:"), sampleDisplaySetting );

         S
            .TieTextBox(XXO("Default audio track &name:"),
               AudioTrackNameSetting, 30);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   S.StartStatic(XO("Zoom Toggle"));
   {
      S.StartMultiColumn(4);
      {
         S
            .TieChoice( XXO("Preset 1:"), zoom1Setting );

         S
            .TieChoice( XXO("Preset 2:"), zoom2Setting );
      }
   }
   S.EndStatic();
   S.EndScroller();
}

bool TracksPrefs::GetPinnedHeadPreference()
{
   // JKC: Cache this setting as it is read many times during drawing, and otherwise causes screen flicker.
   // Correct solution would be to re-write wxFileConfig to be efficient.
   if( iPreferencePinned >= 0 )
      return iPreferencePinned == 1;
   bool bResult = gPrefs->ReadBool(PinnedHeadPreferenceKey(), PinnedHeadPreferenceDefault());
   iPreferencePinned = bResult ? 1: 0;
   return bResult;
}

void TracksPrefs::SetPinnedHeadPreference(bool value, bool flush)
{
   iPreferencePinned = value ? 1 :0;
   gPrefs->Write(PinnedHeadPreferenceKey(), value);
   if(flush)
      gPrefs->Flush();
}

double TracksPrefs::GetPinnedHeadPositionPreference()
{
   auto value = gPrefs->ReadDouble(
      PinnedHeadPositionPreferenceKey(),
      PinnedHeadPositionPreferenceDefault());
   return std::max(0.0, std::min(1.0, value));
}

void TracksPrefs::SetPinnedHeadPositionPreference(double value, bool flush)
{
   value = std::max(0.0, std::min(1.0, value));
   gPrefs->Write(PinnedHeadPositionPreferenceKey(), value);
   if(flush)
      gPrefs->Flush();
}

bool TracksPrefs::Commit()
{
   // Bug 1583: Clear the caching of the preference pinned state.
   iPreferencePinned = -1;
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   // Bug 1661: Don't store the name for new tracks if the name is the
   // default in that language.
   if (WaveTrack::GetDefaultAudioTrackNamePreference() ==
       AudioTrackNameSetting.GetDefault()) {
      AudioTrackNameSetting.Delete();
      gPrefs->Flush();
   }

   return true;
}

namespace{
PrefsPanel::Registration sAttachment{ "Tracks",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew TracksPrefs(parent, winid);
   }
};
}
