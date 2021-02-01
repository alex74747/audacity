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
   Populate();
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

void TracksBehaviorsPrefs::Populate()
{
   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
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

   S.StartStatic(XO("Behaviors"));
   {
      S.TieCheckBox(XXO("&Select all audio, if selection required"),
                    {L"/GUI/SelectAllOnNone",
                     false});
      /* i18n-hint: Cut-lines are lines that can expand to show the cut audio.*/
      S.TieCheckBox(XXO("Enable cut &lines"),
                    {L"/GUI/EnableCutLines",
                     false});
      S.TieCheckBox(XXO("Enable &dragging selection edges"),
                    {L"/GUI/AdjustSelectionEdges",
                     true});
      S
         .TieCheckBox(XXO("Editing a clip can &move other clips"),
            EditClipsCanMove);
      S.TieCheckBox(XXO("\"Move track focus\" c&ycles repeatedly through tracks"),
                    {L"/GUI/CircularTrackNavigation",
                     false});
      S.TieCheckBox(XXO("&Type to create a label"),
                    {L"/GUI/TypeToCreateLabel",
                     false});
      S.TieCheckBox(XXO("Use dialog for the &name of a new label"),
                    {L"/GUI/DialogForNameNewLabel",
                     false});
#ifdef EXPERIMENTAL_SCROLLING_LIMITS
      S.TieCheckBox(XXO("Enable scrolling left of &zero"),
                    ScrollingPreference);
#endif
      S.TieCheckBox(XXO("Advanced &vertical zooming"),
                    {L"/GUI/VerticalZooming",
                     false});

      S.AddSpace(10);

      S.StartMultiColumn(2);
      {
         S.TieChoice( XXO("Solo &Button:"), TracksBehaviorsSolo);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
   S.EndScroller();
}

bool TracksBehaviorsPrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

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
