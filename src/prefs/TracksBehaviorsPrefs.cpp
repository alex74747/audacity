/**********************************************************************

  Audacity: A Digital Audio Editor

  TracksBehaviorsPrefs.cpp

  Steve Daulton


*******************************************************************//**

\class TracksBehaviorsPrefs
\brief A PrefsPanel for Tracks Behaviors settings.

*//*******************************************************************/


#include "TracksBehaviorsPrefs.h"
#include "ViewInfo.h"
#include "WaveTrack.h"

#include "Prefs.h"
#include "../ShuttleGui.h"

TracksBehaviorsPrefs::TracksBehaviorsPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: i.e. the behaviors of tracks */
:  PrefsPanel(parent, winid, XO("Tracks Behaviors"))
{
}

TracksBehaviorsPrefs::~TracksBehaviorsPrefs()
{
}

ComponentInterfaceSymbol TracksBehaviorsPrefs::GetSymbol()
{
   return TRACKS_BEHAVIORS_PREFS_PLUGIN_SYMBOL;
}

TranslatableString TracksBehaviorsPrefs::GetDescription()
{
   return XO("Preferences for TracksBehaviors");
}

ManualPageID TracksBehaviorsPrefs::HelpPageName()
{
   return "Tracks_Behaviors_Preferences";
}

ChoiceSetting TracksBehaviorsSolo{
   L"/GUI/Solo",
   {
      ByColumns,
      { XO("Simple"),  XO("Multi-track"), XO("None") },
      { L"Simple", L"Multi",      L"None" }
   },
   0, // "Simple"
};

void TracksBehaviorsPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S
      .StartStatic(XO("Behaviors"));
   {
      S
         .Target( TracksBehaviorsSelectAllOnNone )
         .AddCheckBox( XXO("&Select all audio, if selection required") );

      S
         .Target( TracksBehaviorsCutLines )
          /* i18n-hint: Cut-lines are lines that can expand to show the cut audio.*/
         .AddCheckBox( XXO("Enable cut &lines") );

      S
         .Target( TracksBehaviorsAdjustSelectionEdges )
         .AddCheckBox( XXO("Enable &dragging selection edges") );

      S
         .Target( EditClipsCanMove )
         .AddCheckBox( XXO("Editing a clip can &move other clips") );

      S
         .Target( TracksBehaviorsCircularNavigation )
         .AddCheckBox( XXO("\"Move track focus\" c&ycles repeatedly through tracks") );

      S
         .Target( TracksBehaviorsTypeToCreateLabel )
         .AddCheckBox( XXO("&Type to create a label") );

      S
         .Target( TracksBehaviorsDialogForNameNewLabel )
         .AddCheckBox( XXO("Use dialog for the &name of a new label") );

#ifdef EXPERIMENTAL_SCROLLING_LIMITS
      S
         .Target( ScrollingPreference )
         .AddCheckBox( XXO("Enable scrolling left of &zero") );
#endif

      S
         .Target( TracksBehaviorsAdvancedVerticalZooming )
         .AddCheckBox( XXO("Advanced &vertical zooming") );

      S.AddSpace(10);

      S.StartMultiColumn(2);
      {
         S
            .Target( TracksBehaviorsSolo )
            .AddChoice( XXO("Solo &Button:") );
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
   S.EndScroller();
}

bool TracksBehaviorsPrefs::Commit()
{
   wxPanel::TransferDataFromWindow();

   return true;
}

namespace{
PrefsPanel::Registration sAttachment{ "TracksBehaviors",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew TracksBehaviorsPrefs(parent, winid);
   },
   false,
   // Place it at a lower tree level
   { "Tracks" }
};
}

BoolSetting TracksBehaviorsCircularNavigation{
   L"/GUI/CircularTrackNavigation", false };
BoolSetting TracksBehaviorsCutLines{
   L"/GUI/EnableCutLines",          false };
BoolSetting TracksBehaviorsDialogForNameNewLabel{
   L"/GUI/DialogForNameNewLabel",   false };
BoolSetting TracksBehaviorsSelectAllOnNone{
   L"/GUI/SelectAllOnNone",         false };
BoolSetting TracksBehaviorsSyncLockTracks{
   L"/GUI/SyncLockTracks",          false  };
BoolSetting TracksBehaviorsTypeToCreateLabel{
   L"/GUI/TypeToCreateLabel",       false };
BoolSetting TracksBehaviorsAdvancedVerticalZooming{
   L"/GUI/VerticalZooming",         false };
