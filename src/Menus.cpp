/**********************************************************************

  Audacity: A Digital Audio Editor

  Menus.cpp

  Dominic Mazzoni
  Brian Gunlogson
  et al.

*******************************************************************//**

\file Menus.cpp
\brief All AudacityProject functions that provide the menus.
Implements AudacityProjectCommandFunctor.

  This file implements the method that creates the menu bar, plus
  all of the methods that get called when you select an item
  from a menu.

  All of the menu bar handling is part of the class AudacityProject,
  but the event handlers for all of the menu items have been moved
  to Menus.h and Menus.cpp for clarity.

*//****************************************************************//**

\class AudacityProjectCommandFunctor
\brief AudacityProjectCommandFunctor, derived from CommandFunctor,
simplifies construction of menu items.

*//*******************************************************************/

#include "Audacity.h"
#include "Project.h"

#include "menus/EditMenuCommands.h"
#include "menus/ViewMenuCommands.h"
#include "menus/TransportMenuCommands.h"
#include "menus/TracksMenuCommands.h"
#include "menus/HelpMenuCommands.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <math.h>


#include <wx/defs.h>
#include <wx/docview.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/textdlg.h>
#include <wx/progdlg.h>
#include <wx/scrolbar.h>
#include <wx/ffile.h>
#include <wx/statusbr.h>
#include <wx/utils.h>

#include "FreqWindow.h"
#include "effects/Contrast.h"
#include "TrackPanel.h"

#include "effects/EffectManager.h"

#include "AudacityApp.h"
#include "AudioIO.h"
#include "Dependencies.h"
#include "float_cast.h"
#include "LabelTrack.h"
#ifdef USE_MIDI
#include "import/ImportMIDI.h"
#endif // USE_MIDI
#include "import/ImportRaw.h"
#include "export/Export.h"
#include "export/ExportMultiple.h"
#include "MixerBoard.h"
#include "Internat.h"
#include "FileFormats.h"
#include "ModuleManager.h"
#include "PluginManager.h"
#include "Prefs.h"
#include "Printing.h"
#ifdef USE_MIDI
#include "NoteTrack.h"
#endif // USE_MIDI
#include "Tags.h"
#include "ondemand/ODManager.h"

#include "Resample.h"
#include "BatchProcessDialog.h"
#include "BatchCommands.h"
#include "prefs/BatchPrefs.h"

#include "commands/CommandManager.h"

#include "toolbars/ToolManager.h"
#include "toolbars/ControlToolBar.h"
#include "toolbars/ToolsToolBar.h"
#include "toolbars/EditToolBar.h"
#include "toolbars/DeviceToolBar.h"
#include "toolbars/MixerToolBar.h"
#include "toolbars/TranscriptionToolBar.h"

#include "Experimental.h"
#include "PlatformCompatibility.h"
#include "FileNames.h"

#include "SplashDialog.h"

#include "Snap.h"

#include "WaveTrack.h"

#if defined(EXPERIMENTAL_CRASH_REPORT)
#include <wx/debugrpt.h>
#endif

#ifdef EXPERIMENTAL_SCOREALIGN
#include "effects/ScoreAlignDialog.h"
#include "audioreader.h"
#include "scorealign.h"
#include "scorealign-glue.h"
#endif /* EXPERIMENTAL_SCOREALIGN */

typedef ObjectCommandFunctor<AudacityProject> AudacityProjectCommandFunctor;

#define FN(X) new AudacityProjectCommandFunctor(this, &AudacityProject:: X )
#define FNI(X, I) new AudacityProjectCommandFunctor(this, &AudacityProject:: X, I)
#define FNS(X, S) new AudacityProjectCommandFunctor(this, &AudacityProject:: X, S)

//
// Effects menu arrays
//
WX_DEFINE_ARRAY_PTR(const PluginDescriptor *, EffectPlugs);
static int SortEffectsByName(const PluginDescriptor **a, const PluginDescriptor **b)
{
   wxString akey = (*a)->GetName();
   wxString bkey = (*b)->GetName();

   akey += (*a)->GetPath();
   bkey += (*b)->GetPath();

   return akey.CmpNoCase(bkey);
}

static int SortEffectsByPublisher(const PluginDescriptor **a, const PluginDescriptor **b)
{
   wxString akey = (*a)->GetVendor();
   wxString bkey = (*b)->GetVendor();

   if (akey.IsEmpty())
   {
      akey = _("Uncategorized");
   }
   if (bkey.IsEmpty())
   {
      bkey = _("Uncategorized");
   }

   akey += (*a)->GetName();
   bkey += (*b)->GetName();

   akey += (*a)->GetPath();
   bkey += (*b)->GetPath();

   return akey.CmpNoCase(bkey);
}

static int SortEffectsByPublisherAndName(const PluginDescriptor **a, const PluginDescriptor **b)
{
   wxString akey = (*a)->GetVendor();
   wxString bkey = (*b)->GetVendor();

   if ((*a)->IsEffectDefault())
   {
      akey = wxEmptyString;
   }
   if ((*b)->IsEffectDefault())
   {
      bkey = wxEmptyString;
   }

   akey += (*a)->GetName();
   bkey += (*b)->GetName();

   akey += (*a)->GetPath();
   bkey += (*b)->GetPath();

   return akey.CmpNoCase(bkey);
}

static int SortEffectsByTypeAndName(const PluginDescriptor **a, const PluginDescriptor **b)
{
   wxString akey = (*a)->GetEffectFamily();
   wxString bkey = (*b)->GetEffectFamily();

   if (akey.IsEmpty())
   {
      akey = _("Uncategorized");
   }
   if (bkey.IsEmpty())
   {
      bkey = _("Uncategorized");
   }

   if ((*a)->IsEffectDefault())
   {
      akey = wxEmptyString;
   }
   if ((*b)->IsEffectDefault())
   {
      bkey = wxEmptyString;
   }

   akey += (*a)->GetName();
   bkey += (*b)->GetName();

   akey += (*a)->GetPath();
   bkey += (*b)->GetPath();

   return akey.CmpNoCase(bkey);
}

static int SortEffectsByType(const PluginDescriptor **a, const PluginDescriptor **b)
{
   wxString akey = (*a)->GetEffectFamily();
   wxString bkey = (*b)->GetEffectFamily();

   if (akey.IsEmpty())
   {
      akey = _("Uncategorized");
   }
   if (bkey.IsEmpty())
   {
      bkey = _("Uncategorized");
   }

   akey += (*a)->GetName();
   bkey += (*b)->GetName();

   akey += (*a)->GetPath();
   bkey += (*b)->GetPath();

   return akey.CmpNoCase(bkey);
}

/// CreateMenusAndCommands builds the menus, and also rebuilds them after
/// changes in configured preferences - for example changes in key-bindings
/// affect the short-cut key legend that appears beside each command,

void AudacityProject::CreateMenusAndCommands()
{
   CommandManager *c = mCommandManager;
   wxArrayString names;
   wxArrayInt indices;

   wxMenuBar *menubar = c->AddMenuBar(wxT("appmenu"));

   /////////////////////////////////////////////////////////////////////////////
   // File menu
   /////////////////////////////////////////////////////////////////////////////

   c->BeginMenu(_("&File"));
   c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

   /*i18n-hint: "New" is an action (verb) to create a new project*/
   c->AddItem(wxT("New"), _("&New"), FN(OnNew), wxT("Ctrl+N"),
              AudioIONotBusyFlag,
              AudioIONotBusyFlag);

   /*i18n-hint: (verb)*/
   c->AddItem(wxT("Open"), _("&Open..."), FN(OnOpen), wxT("Ctrl+O"),
              AudioIONotBusyFlag,
              AudioIONotBusyFlag);

   /////////////////////////////////////////////////////////////////////////////

   CreateRecentFilesMenu(c);

   /////////////////////////////////////////////////////////////////////////////

   c->AddSeparator();

   c->AddItem(wxT("Close"), _("&Close"), FN(OnClose), wxT("Ctrl+W"));

   c->AddItem(wxT("Save"), _("&Save Project"), FN(OnSave), wxT("Ctrl+S"),
              AudioIONotBusyFlag | UnsavedChangesFlag,
              AudioIONotBusyFlag | UnsavedChangesFlag);
   c->AddItem(wxT("SaveAs"), _("Save Project &As..."), FN(OnSaveAs));
#ifdef USE_LIBVORBIS
   c->AddItem(wxT("SaveCompressed"), _("Save Compressed Copy of Project..."), FN(OnSaveCompressed));
#endif

   c->AddItem(wxT("CheckDeps"), _("Chec&k Dependencies..."), FN(OnCheckDependencies));

   c->AddSeparator();

   c->AddItem(wxT("EditMetaData"), _("Edit Me&tadata..."), FN(OnEditMetadata));

   c->AddSeparator();

   /////////////////////////////////////////////////////////////////////////////

   c->BeginSubMenu(_("&Import"));

   c->AddItem(wxT("ImportAudio"), _("&Audio..."), FN(OnImport), wxT("Ctrl+Shift+I"));
   c->AddItem(wxT("ImportLabels"), _("&Labels..."), FN(OnImportLabels));
#ifdef USE_MIDI
   c->AddItem(wxT("ImportMIDI"), _("&MIDI..."), FN(OnImportMIDI));
#endif // USE_MIDI
   c->AddItem(wxT("ImportRaw"), _("&Raw Data..."), FN(OnImportRaw));

   c->EndSubMenu();

   c->AddSeparator();

   /////////////////////////////////////////////////////////////////////////////

   // Enable Export audio commands only when there are audio tracks.
   c->AddItem(wxT("Export"), _("&Export Audio..."), FN(OnExport), wxT("Ctrl+Shift+E"),
              AudioIONotBusyFlag | WaveTracksExistFlag,
              AudioIONotBusyFlag | WaveTracksExistFlag);

   // Enable Export Selection commands only when there's a selection.
   c->AddItem(wxT("ExportSel"), _("Expo&rt Selected Audio..."), FN(OnExportSelection),
              AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
              AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag);

   c->AddItem(wxT("ExportLabels"), _("Export &Labels..."), FN(OnExportLabels),
              AudioIONotBusyFlag | LabelTracksExistFlag,
              AudioIONotBusyFlag | LabelTracksExistFlag);
   // Enable Export audio commands only when there are audio tracks.
   c->AddItem(wxT("ExportMultiple"), _("Export &Multiple..."), FN(OnExportMultiple), wxT("Ctrl+Shift+L"),
              AudioIONotBusyFlag | WaveTracksExistFlag,
              AudioIONotBusyFlag | WaveTracksExistFlag);
#if defined(USE_MIDI)
   c->AddItem(wxT("ExportMIDI"),   _("Export MIDI..."), FN(OnExportMIDI),
              AudioIONotBusyFlag | NoteTracksSelectedFlag,
              AudioIONotBusyFlag | NoteTracksSelectedFlag);
#endif

   c->AddSeparator();
   c->AddItem(wxT("ApplyChain"), _("Appl&y Chain..."), FN(OnApplyChain),
              AudioIONotBusyFlag,
              AudioIONotBusyFlag);
   c->AddItem(wxT("EditChains"), _("Edit C&hains..."), FN(OnEditChains));

   c->AddSeparator();

   c->AddItem(wxT("PageSetup"), _("Pa&ge Setup..."), FN(OnPageSetup),
              AudioIONotBusyFlag | TracksExistFlag,
              AudioIONotBusyFlag | TracksExistFlag);
   /* i18n-hint: (verb) It's item on a menu. */
   c->AddItem(wxT("Print"), _("&Print..."), FN(OnPrint),
              AudioIONotBusyFlag | TracksExistFlag,
              AudioIONotBusyFlag | TracksExistFlag);

   c->AddSeparator();

   // On the Mac, the Exit item doesn't actually go here...wxMac will pull it out
   // and put it in the Audacity menu for us based on its ID.
   /* i18n-hint: (verb) It's item on a menu. */
   c->AddItem(wxT("Exit"), _("E&xit"), FN(OnExit), wxT("Ctrl+Q"),
              AlwaysEnabledFlag,
              AlwaysEnabledFlag);

   c->EndMenu();

   /////////////////////////////////////////////////////////////////////////////
   // Edit Menu
   /////////////////////////////////////////////////////////////////////////////

   mEditMenuCommands->Create(c);

   /////////////////////////////////////////////////////////////////////////////
   // View Menu
   /////////////////////////////////////////////////////////////////////////////

   mViewMenuCommands->Create(c);

   /////////////////////////////////////////////////////////////////////////////
   // Transport Menu
   /////////////////////////////////////////////////////////////////////////////

   mTransportMenuCommands->Create(c);

   //////////////////////////////////////////////////////////////////////////
   // Tracks Menu (formerly Project Menu)
   //////////////////////////////////////////////////////////////////////////

   mTracksMenuCommands->Create(c);

   // All of this is a bit hacky until we can get more things connected into
   // the plugin manager...sorry! :-(

   wxArrayString defaults;

   //////////////////////////////////////////////////////////////////////////
   // Generate Menu
   //////////////////////////////////////////////////////////////////////////

   c->BeginMenu(_("&Generate"));
   c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

#ifdef EXPERIMENTAL_EFFECT_MANAGEMENT
   c->AddItem(wxT("ManageGenerators"), _("Add / Remove Plug-ins..."), FN(OnManageGenerators));
   c->AddSeparator();
#endif


   PopulateEffectsMenu(c,
                       EffectTypeGenerate,
                       AudioIONotBusyFlag,
                       AudioIONotBusyFlag);

   c->EndMenu();

   /////////////////////////////////////////////////////////////////////////////
   // Effect Menu
   /////////////////////////////////////////////////////////////////////////////

   c->BeginMenu(_("Effe&ct"));

   wxString buildMenuLabel;
   if (!mLastEffect.IsEmpty()) {
      buildMenuLabel.Printf(_("Repeat %s"),
                            EffectManager::Get().GetEffectName(mLastEffect).c_str());
   }
   else
      buildMenuLabel.Printf(_("Repeat Last Effect"));

#ifdef EXPERIMENTAL_EFFECT_MANAGEMENT
   c->AddItem(wxT("ManageEffects"), _("Add / Remove Plug-ins..."), FN(OnManageEffects));
   c->AddSeparator();
#endif

   c->AddItem(wxT("RepeatLastEffect"), buildMenuLabel, FN(OnRepeatLastEffect), wxT("Ctrl+R"),
              AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag | HasLastEffectFlag,
              AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag | HasLastEffectFlag);

   c->AddSeparator();

   PopulateEffectsMenu(c,
                       EffectTypeProcess,
                       AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
                       IsRealtimeNotActiveFlag);

   c->EndMenu();

   //////////////////////////////////////////////////////////////////////////
   // Analyze Menu
   //////////////////////////////////////////////////////////////////////////

   c->BeginMenu(_("&Analyze"));

#ifdef EXPERIMENTAL_EFFECT_MANAGEMENT
   c->AddItem(wxT("ManageAnalyzers"), _("Add / Remove Plug-ins..."), FN(OnManageAnalyzers));
   c->AddSeparator();
#endif


   c->AddItem(wxT("ContrastAnalyser"), _("Contrast..."), FN(OnContrast), wxT("Ctrl+Shift+T"),
              AudioIONotBusyFlag | WaveTracksSelectedFlag | TimeSelectedFlag,
              AudioIONotBusyFlag | WaveTracksSelectedFlag | TimeSelectedFlag);
   c->AddItem(wxT("PlotSpectrum"), _("Plot Spectrum..."), FN(OnPlotSpectrum),
              AudioIONotBusyFlag | WaveTracksSelectedFlag | TimeSelectedFlag,
              AudioIONotBusyFlag | WaveTracksSelectedFlag | TimeSelectedFlag);

   PopulateEffectsMenu(c,
                       EffectTypeAnalyze,
                       AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
                       IsRealtimeNotActiveFlag);

   c->EndMenu();

   /////////////////////////////////////////////////////////////////////////////
   // Help Menu
   /////////////////////////////////////////////////////////////////////////////

   #ifdef __WXMAC__
   wxGetApp().s_macHelpMenuTitleName = _("&Help");
   #endif

   mHelpMenuCommands->Create(c);

   /////////////////////////////////////////////////////////////////////////////

   SetMenuBar(menubar);

   mEditMenuCommands->CreateNonMenuCommands(c);
   mViewMenuCommands->CreateNonMenuCommands(c);
   mTransportMenuCommands->CreateNonMenuCommands(c);
   mTracksMenuCommands->CreateNonMenuCommands(c);

   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddCommand(wxT("SelectTool"), _("Selection Tool"), FN(OnSelectTool), wxT("F1"));
   c->AddCommand(wxT("EnvelopeTool"),_("Envelope Tool"), FN(OnEnvelopeTool), wxT("F2"));
   c->AddCommand(wxT("DrawTool"), _("Draw Tool"), FN(OnDrawTool), wxT("F3"));
   c->AddCommand(wxT("ZoomTool"), _("Zoom Tool"), FN(OnZoomTool), wxT("F4"));
   c->AddCommand(wxT("TimeShiftTool"), _("Time Shift Tool"), FN(OnTimeShiftTool), wxT("F5"));
   c->AddCommand(wxT("MultiTool"), _("Multi Tool"), FN(OnMultiTool), wxT("F6"));

   c->AddCommand(wxT("NextTool"), _("Next Tool"), FN(OnNextTool), wxT("D"));
   c->AddCommand(wxT("PrevTool"), _("Previous Tool"), FN(OnPrevTool), wxT("A"));

   c->SetDefaultFlags(TracksExistFlag | TrackPanelHasFocus,
                      TracksExistFlag | TrackPanelHasFocus);

   c->AddCommand(wxT("CursorLongJumpLeft"), _("Cursor Long Jump Left"), FN(OnCursorLongJumpLeft), wxT("Shift+,"));
   c->AddCommand(wxT("CursorLongJumpRight"), _("Cursor Long Jump Right"), FN(OnCursorLongJumpRight), wxT("Shift+."));

   c->AddCommand(wxT("SelExtLeft"), _("Selection Extend Left"), FN(OnSelExtendLeft), wxT("Shift+Left\twantKeyup\tallowDup"));
   c->AddCommand(wxT("SelExtRight"), _("Selection Extend Right"), FN(OnSelExtendRight), wxT("Shift+Right\twantKeyup\tallowDup"));

   c->AddCommand(wxT("SelSetExtLeft"), _("Set (or Extend) Left Selection"), FN(OnSelSetExtendLeft));
   c->AddCommand(wxT("SelSetExtRight"), _("Set (or Extend) Right Selection"), FN(OnSelSetExtendRight));

   c->AddCommand(wxT("SelCntrLeft"), _("Selection Contract Left"), FN(OnSelContractLeft), wxT("Ctrl+Shift+Right\twantKeyup"));
   c->AddCommand(wxT("SelCntrRight"), _("Selection Contract Right"), FN(OnSelContractRight), wxT("Ctrl+Shift+Left\twantKeyup"));

   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddCommand(wxT("SnapToOff"), _("Snap To Off"), FN(OnSnapToOff));
   c->AddCommand(wxT("SnapToNearest"), _("Snap To Nearest"), FN(OnSnapToNearest));
   c->AddCommand(wxT("SnapToPrior"), _("Snap To Prior"), FN(OnSnapToPrior));

   mLastFlags = 0;

#if defined(__WXDEBUG__)
//   c->CheckDups();
#endif
}

void AudacityProject::PopulateEffectsMenu(CommandManager* c,
                                          EffectType type,
                                          int batchflags,
                                          int realflags)
{
   PluginManager & pm = PluginManager::Get();

   EffectPlugs defplugs;
   EffectPlugs optplugs;

   const PluginDescriptor *plug = pm.GetFirstPluginForEffectType(type);
   while (plug)
   {
      if ( !plug->IsEnabled() ){
         ;// don't add to menus!
      }
      else if (plug->IsEffectDefault())
      {
         defplugs.Add(plug);
      }
      else
      {
         optplugs.Add(plug);
      }
      plug = pm.GetNextPluginForEffectType(type);
   }

   wxString groupby = gPrefs->Read(wxT("/Effects/GroupBy"), wxT("name"));

   if (groupby == wxT("sortby:name"))
   {
      defplugs.Sort(SortEffectsByName);
      optplugs.Sort(SortEffectsByName);
   }
   else if (groupby == wxT("sortby:publisher:name"))
   {
      defplugs.Sort(SortEffectsByName);
      optplugs.Sort(SortEffectsByPublisherAndName);
   }
   else if (groupby == wxT("sortby:type:name"))
   {
      defplugs.Sort(SortEffectsByName);
      optplugs.Sort(SortEffectsByTypeAndName);
   }
   else if (groupby == wxT("groupby:publisher"))
   {
      defplugs.Sort(SortEffectsByPublisher);
      optplugs.Sort(SortEffectsByPublisher);
   }
   else if (groupby == wxT("groupby:type"))
   {
      defplugs.Sort(SortEffectsByType);
      optplugs.Sort(SortEffectsByType);
   }      
   else // name
   {
      defplugs.Sort(SortEffectsByName);
      optplugs.Sort(SortEffectsByName);
   }

   AddEffectMenuItems(c, defplugs, batchflags, realflags, true);

   if (defplugs.GetCount() && optplugs.GetCount())
   {
      c->AddSeparator();
   }

   AddEffectMenuItems(c, optplugs, batchflags, realflags, false);

   return;
}

void AudacityProject::AddEffectMenuItems(CommandManager *c,
                                         EffectPlugs & plugs,
                                         int batchflags,
                                         int realflags,
                                         bool isDefault)
{
   size_t pluginCnt = plugs.GetCount();

   wxString groupBy = gPrefs->Read(wxT("/Effects/GroupBy"), wxT("name"));

   bool grouped = false;
   if (groupBy.StartsWith(wxT("groupby")))
   {
      grouped = true;
   }

   wxArrayString groupNames;
   PluginIDList groupPlugs;
   wxArrayInt groupFlags;
   if (grouped)
   {
      wxString last;
      wxString current;

      for (size_t i = 0; i < pluginCnt; i++)
      {
         const PluginDescriptor *plug = plugs[i];

         wxString name = plug->GetName();

         if (plug->IsEffectInteractive())
         {
            name += wxT("...");
         }

         if (groupBy == wxT("groupby:publisher"))
         {
            current = plug->GetVendor();
            if (current.IsEmpty())
            {
               current = _("Unknown");
            }
         }
         else if (groupBy == wxT("groupby:type"))
         {
            current = plug->GetEffectFamily();
            if (current.IsEmpty())
            {
               current = _("Unknown");
            }
         }

         if (current != last)
         {
            if (!last.IsEmpty())
            {
               c->BeginSubMenu(last);
            }

            AddEffectMenuItemGroup(c, groupNames, groupPlugs, groupFlags, isDefault);

            if (!last.IsEmpty())
            {
               c->EndSubMenu();
            }

            groupNames.Clear();
            groupPlugs.Clear();
            groupFlags.Clear();
            last = current;
         }

         groupNames.Add(name);
         groupPlugs.Add(plug->GetID());
         groupFlags.Add(plug->IsEffectRealtime() ? realflags : batchflags);
      }

      if (groupNames.GetCount() > 0)
      {
         c->BeginSubMenu(current);

         AddEffectMenuItemGroup(c, groupNames, groupPlugs, groupFlags, isDefault);

         c->EndSubMenu();
      }
   }
   else
   {
      for (size_t i = 0; i < pluginCnt; i++)
      {
         const PluginDescriptor *plug = plugs[i];

         wxString name = plug->GetName();

         if (plug->IsEffectInteractive())
         {
            name += wxT("...");
         }

         wxString group = wxEmptyString;
         if (groupBy == wxT("sortby:publisher:name"))
         {
            group = plug->GetVendor();
         }
         else if (groupBy == wxT("sortby:type:name"))
         {
            group = plug->GetEffectFamily();
         }

         if (plug->IsEffectDefault())
         {
            group = wxEmptyString;
         }

         if (!group.IsEmpty())
         {
            group += wxT(": ");
         }

         groupNames.Add(group + name);
         groupPlugs.Add(plug->GetID());
         groupFlags.Add(plug->IsEffectRealtime() ? realflags : batchflags);
      }

      if (groupNames.GetCount() > 0)
      {
         AddEffectMenuItemGroup(c, groupNames, groupPlugs, groupFlags, isDefault);
      }

   }

   return;
}

void AudacityProject::AddEffectMenuItemGroup(CommandManager *c,
                                             const wxArrayString & names,
                                             const PluginIDList & plugs,
                                             const wxArrayInt & flags,
                                             bool isDefault)
{
   int namesCnt = (int) names.GetCount();
   int perGroup;

#if defined(__WXGTK__)
   gPrefs->Read(wxT("/Effects/MaxPerGroup"), &perGroup, 15);
#else
   gPrefs->Read(wxT("/Effects/MaxPerGroup"), &perGroup, 0);
#endif

   int groupCnt = namesCnt;
   for (int i = 0; i < namesCnt; i++)
   {
      while (i + 1 < namesCnt && names[i].IsSameAs(names[i + 1]))
      {
         i++;
         groupCnt--;
      }
   }

   // The "default" effects shouldn't be broken into subgroups
   if (namesCnt > 0 && isDefault)
   {
      perGroup = 0;
   }

   int max = perGroup;
   int items = perGroup;

   if (max > groupCnt)
   {
      max = 0;
   }

   int groupNdx = 0;
   for (int i = 0; i < namesCnt; i++)
   {
      if (max > 0 && items == max)
      {
         int end = groupNdx + max;
         if (end + 1 > groupCnt)
         {
            end = groupCnt;
         }
         c->BeginSubMenu(wxString::Format(_("Plug-ins %d to %d"),
                                          groupNdx + 1,
                                          end));
      }

      if (i + 1 < namesCnt && names[i].IsSameAs(names[i + 1]))
      {
         wxString name = names[i];
         c->BeginSubMenu(name);
         while (i < namesCnt && names[i].IsSameAs(name))
         {
            wxString item = PluginManager::Get().GetPlugin(plugs[i])->GetPath();
            c->AddItem(item,
                       item,
                       FNS(OnEffect, plugs[i]),
                       flags[i],
                       flags[i]);

            i++;
         }
         c->EndSubMenu();
         i--;
      }
      else
      {
         c->AddItem(names[i],
                    names[i],
                    FNS(OnEffect, plugs[i]),
                    flags[i],
                    flags[i]);
      }

      if (max > 0)
      {
         groupNdx++;
         items--;
         if (items == 0 || i + 1 == namesCnt)
         {
            c->EndSubMenu();
            items = max;
         }
      }
   }

   return;
}

void AudacityProject::CreateRecentFilesMenu(CommandManager *c)
{
   // Recent Files and Recent Projects menus

#ifdef __WXMAC__
   /* i18n-hint: This is the name of the menu item on Mac OS X only */
   mRecentFilesMenu = c->BeginSubMenu(_("Open Recent"));
#else
   /* i18n-hint: This is the name of the menu item on Windows and Linux */
   mRecentFilesMenu = c->BeginSubMenu(_("Recent &Files"));
#endif

   wxGetApp().GetRecentFiles()->UseMenu(mRecentFilesMenu);
   wxGetApp().GetRecentFiles()->AddFilesToMenu(mRecentFilesMenu);

   c->EndSubMenu();

}

void AudacityProject::ModifyUndoMenuItems()
{
   wxString desc;
   int cur = mUndoManager.GetCurrentState();

   if (mUndoManager.UndoAvailable()) {
      mUndoManager.GetShortDescription(cur, &desc);
      GetCommandManager()->Modify(wxT("Undo"),
                             wxString::Format(_("&Undo %s"),
                                              desc.c_str()));
   }
   else {
      GetCommandManager()->Modify(wxT("Undo"),
                             wxString::Format(_("&Undo")));
   }

   if (mUndoManager.RedoAvailable()) {
      mUndoManager.GetShortDescription(cur+1, &desc);
      GetCommandManager()->Modify(wxT("Redo"),
                             wxString::Format(_("&Redo %s"),
                                              desc.c_str()));
      GetCommandManager()->Enable(wxT("Redo"), true);
   }
   else {
      GetCommandManager()->Modify(wxT("Redo"),
                             wxString::Format(_("&Redo")));
      GetCommandManager()->Enable(wxT("Redo"), false);
   }
}

void AudacityProject::RebuildMenuBar()
{
   // On OSX, we can't rebuild the menus while a modal dialog is being shown
   // since the enabled state for menus like Quit and Preference gets out of
   // sync with wxWidgets idea of what it should be.
#if defined(__WXMAC__) && defined(__WXDEBUG__)
   {
      wxDialog *dlg = wxDynamicCast(wxGetTopLevelParent(FindFocus()), wxDialog);
      wxASSERT((!dlg || !dlg->IsModal()));
   }
#endif

   // Allow FileHistory to remove its own menu
   wxGetApp().GetRecentFiles()->RemoveMenu(mRecentFilesMenu);

   // Delete the menus, since we will soon recreate them.
   // Rather oddly, the menus don't vanish as a result of doing this.
   wxMenuBar *menuBar = GetMenuBar();
   DetachMenuBar();
   delete menuBar;

   GetCommandManager()->PurgeData();

   CreateMenusAndCommands();

   ModuleManager::Get().Dispatch(MenusRebuilt);
}

void AudacityProject::RebuildOtherMenus()
{
   if (mTrackPanel) {
      mTrackPanel->BuildMenus();
   }
}

int AudacityProject::GetFocusedFrame()
{
   wxWindow *w = FindFocus();

   while (w && mToolManager && mTrackPanel) {
      if (w == mToolManager->GetTopDock()) {
         return TopDockHasFocus;
      }

      if (w == mTrackPanel) {
         return TrackPanelHasFocus;
      }

      if (w == mToolManager->GetBotDock()) {
         return BotDockHasFocus;
      }

      w = w->GetParent();
   }

   return 0;
}

wxUint32 AudacityProject::GetUpdateFlags()
{
   // This method determines all of the flags that determine whether
   // certain menu items and commands should be enabled or disabled,
   // and returns them in a bitfield.  Note that if none of the flags
   // have changed, it's not necessary to even check for updates.
   wxUint32 flags = 0;

   if (!gAudioIO->IsAudioTokenActive(GetAudioIOToken()))
      flags |= AudioIONotBusyFlag;
   else
      flags |= AudioIOBusyFlag;

   if (!mViewInfo.selectedRegion.isPoint())
      flags |= TimeSelectedFlag;

   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   while (t) {
      flags |= TracksExistFlag;
      if (t->GetKind() == Track::Label) {
         LabelTrack *lt = (LabelTrack *) t;

         flags |= LabelTracksExistFlag;

         if (lt->GetSelected()) {
            flags |= TracksSelectedFlag;
            for (int i = 0; i < lt->GetNumLabels(); i++) {
               const LabelStruct *ls = lt->GetLabel(i);
               if (ls->getT0() >= mViewInfo.selectedRegion.t0() &&
                   ls->getT1() <= mViewInfo.selectedRegion.t1()) {
                  flags |= LabelsSelectedFlag;
                  break;
               }
            }
         }

         if (lt->IsTextSelected()) {
            flags |= CutCopyAvailableFlag;
         }
      }
      else if (t->GetKind() == Track::Wave) {
         flags |= WaveTracksExistFlag;
         if (t->GetSelected()) {
            flags |= TracksSelectedFlag;
            if (t->GetLinked()) {
               flags |= StereoRequiredFlag;
            }
            else {
               flags |= WaveTracksSelectedFlag;
            }
         }
      }
#if defined(USE_MIDI)
      else if (t->GetKind() == Track::Note) {
         NoteTrack *nt = (NoteTrack *) t;

         flags |= NoteTracksExistFlag;

         if (nt->GetSelected()) {
            flags |= TracksSelectedFlag;
            flags |= NoteTracksSelectedFlag;
         }
      }
#endif
      t = iter.Next();
   }

   if((msClipT1 - msClipT0) > 0.0)
      flags |= ClipboardFlag;

   if (mUndoManager.UnsavedChanges())
      flags |= UnsavedChangesFlag;

   if (!mLastEffect.IsEmpty())
      flags |= HasLastEffectFlag;

   if (mUndoManager.UndoAvailable())
      flags |= UndoAvailableFlag;

   if (mUndoManager.RedoAvailable())
      flags |= RedoAvailableFlag;

   if (ZoomInAvailable() && (flags & TracksExistFlag))
      flags |= ZoomInAvailableFlag;

   if (ZoomOutAvailable() && (flags & TracksExistFlag))
      flags |= ZoomOutAvailableFlag;

   if ((flags & LabelTracksExistFlag) && LabelTrack::IsTextClipSupported())
      flags |= TextClipFlag;

   flags |= GetFocusedFrame();

   double start, end;
   GetPlayRegion(&start, &end);
   if (IsPlayRegionLocked())
      flags |= PlayRegionLockedFlag;
   else if (start != end)
      flags |= PlayRegionNotLockedFlag;

   if (flags & AudioIONotBusyFlag) {
      if (flags & TimeSelectedFlag) {
         if (flags & TracksSelectedFlag) {
            flags |= CutCopyAvailableFlag;
         }
      }
   }

   if (wxGetApp().GetRecentFiles()->GetCount() > 0)
      flags |= HaveRecentFiles;

   if (IsSyncLocked())
      flags |= IsSyncLockedFlag;
   else
      flags |= IsNotSyncLockedFlag;

   if (!EffectManager::Get().RealtimeIsActive())
      flags |= IsRealtimeNotActiveFlag;

      if (!mIsCapturing)
      flags |= CaptureNotBusyFlag;

   return flags;
}

void AudacityProject::ModifyAllProjectToolbarMenus()
{
   AProjectArray::iterator i;
   for (i = gAudacityProjects.begin(); i != gAudacityProjects.end(); ++i) {
      ((AudacityProject *)*i)->ModifyToolbarMenus();
   }
}

void AudacityProject::ModifyToolbarMenus()
{
   // Refreshes can occur during shutdown and the toolmanager may already
   // be deleted, so protect against it.
   if (!mToolManager) {
      return;
   }

   GetCommandManager()->Check(wxT("ShowDeviceTB"),
                         mToolManager->IsVisible(DeviceBarID));
   GetCommandManager()->Check(wxT("ShowEditTB"),
                         mToolManager->IsVisible(EditBarID));
   GetCommandManager()->Check(wxT("ShowMeterTB"),
                         mToolManager->IsVisible(MeterBarID));
   GetCommandManager()->Check(wxT("ShowRecordMeterTB"),
                         mToolManager->IsVisible(RecordMeterBarID));
   GetCommandManager()->Check(wxT("ShowPlayMeterTB"),
                         mToolManager->IsVisible(PlayMeterBarID));
   GetCommandManager()->Check(wxT("ShowMixerTB"),
                         mToolManager->IsVisible(MixerBarID));
   GetCommandManager()->Check(wxT("ShowSelectionTB"),
                         mToolManager->IsVisible(SelectionBarID));
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   GetCommandManager()->Check(wxT("ShowSpectralSelectionTB"),
                         mToolManager->IsVisible(SpectralSelectionBarID));
#endif
   GetCommandManager()->Check(wxT("ShowToolsTB"),
                         mToolManager->IsVisible(ToolsBarID));
   GetCommandManager()->Check(wxT("ShowTranscriptionTB"),
                         mToolManager->IsVisible(TranscriptionBarID));
   GetCommandManager()->Check(wxT("ShowTransportTB"),
                         mToolManager->IsVisible(TransportBarID));

   // Now, go through each toolbar, and call EnableDisableButtons()
   for (int i = 0; i < ToolBarCount; i++) {
      mToolManager->GetToolBar(i)->EnableDisableButtons();
   }

   // These don't really belong here, but it's easier and especially so for
   // the Edit toolbar and the sync-lock menu item.
   bool active;
   gPrefs->Read(wxT("/AudioIO/SoundActivatedRecord"),&active, false);
   GetCommandManager()->Check(wxT("SoundActivation"), active);
#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
   gPrefs->Read(wxT("/AudioIO/AutomatedInputLevelAdjustment"),&active, false);
   GetCommandManager()->Check(wxT("AutomatedInputLevelAdjustmentOnOff"), active);
#endif
   gPrefs->Read(wxT("/AudioIO/Duplex"),&active, true);
   GetCommandManager()->Check(wxT("Duplex"), active);
   gPrefs->Read(wxT("/AudioIO/SWPlaythrough"),&active, false);
   GetCommandManager()->Check(wxT("SWPlaythrough"), active);
   gPrefs->Read(wxT("/GUI/SyncLockTracks"), &active, false);
   SetSyncLock(active);
   GetCommandManager()->Check(wxT("SyncLock"), active);
}

// checkActive is a temporary hack that should be removed as soon as we
// get multiple effect preview working
void AudacityProject::UpdateMenus(bool checkActive)
{
   //ANSWER-ME: Why UpdateMenus only does active project?
   //JKC: Is this test fixing a bug when multiple projects are open?
   //so that menu states work even when different in different projects?
   if (this != GetActiveProject())
      return;

   if (checkActive && !IsActive())
      return;

   wxUint32 flags = GetUpdateFlags();
   wxUint32 flags2 = flags;

   // We can enable some extra items if we have select-all-on-none.
   //EXPLAIN-ME: Why is this here rather than in GetUpdateFlags()?
   if (mSelectAllOnNone)
   {
      if ((flags & TracksExistFlag) != 0)
      {
         flags2 |= TracksSelectedFlag;
         if ((flags & WaveTracksExistFlag) != 0 )
         {
            flags2 |= TimeSelectedFlag
                   |  WaveTracksSelectedFlag
                   |  CutCopyAvailableFlag;
         }
      }
   }

   // Return from this function if nothing's changed since
   // the last time we were here.
   if (flags == mLastFlags)
      return;
   mLastFlags = flags;

   GetCommandManager()->EnableUsingFlags(flags2 , 0xFFFFFFFF);

   // With select-all-on-none, some items that we don't want enabled may have
   // been enabled, since we changed the flags.  Here we manually disable them.
   if (mSelectAllOnNone)
   {
      if ((flags & TracksSelectedFlag) == 0)
      {
         GetCommandManager()->Enable(wxT("SplitCut"), false);

         if ((flags & WaveTracksSelectedFlag) == 0)
         {
            GetCommandManager()->Enable(wxT("Split"), false);
         }
         if ((flags & TimeSelectedFlag) == 0)
         {
            GetCommandManager()->Enable(wxT("ExportSel"), false);
            GetCommandManager()->Enable(wxT("SplitNew"), false);
            GetCommandManager()->Enable(wxT("Trim"), false);
            GetCommandManager()->Enable(wxT("SplitDelete"), false);
         }
      }
   }

#if 0
   if (flags & CutCopyAvailableFlag) {
      GetCommandManager()->Enable(wxT("Copy"), true);
      GetCommandManager()->Enable(wxT("Cut"), true);
   }
#endif

   ModifyToolbarMenus();
}

//
// Tool selection commands
//

void AudacityProject::SetTool(int tool)
{
   ToolsToolBar *toolbar = GetToolsToolBar();
   if (toolbar) {
      toolbar->SetCurrentTool(tool, true);
      mTrackPanel->Refresh(false);
   }
}

void AudacityProject::OnSelectTool()
{
   SetTool(selectTool);
}

void AudacityProject::OnZoomTool()
{
   SetTool(zoomTool);
}

void AudacityProject::OnEnvelopeTool()
{
   SetTool(envelopeTool);
}

void AudacityProject::OnTimeShiftTool()
{
   SetTool(slideTool);
}

void AudacityProject::OnDrawTool()
{
   SetTool(drawTool);
}

void AudacityProject::OnMultiTool()
{
   SetTool(multiTool);
}


void AudacityProject::OnNextTool()
{
   ToolsToolBar *toolbar = GetToolsToolBar();
   if (toolbar) {
      // Use GetDownTool() here since GetCurrentTool() can return a value that
      // doesn't represent the real tool if the Multi-tool is being used.
      toolbar->SetCurrentTool((toolbar->GetDownTool()+1)%numTools, true);
      mTrackPanel->Refresh(false);
   }
}

void AudacityProject::OnPrevTool()
{
   ToolsToolBar *toolbar = GetToolsToolBar();
   if (toolbar) {
      // Use GetDownTool() here since GetCurrentTool() can return a value that
      // doesn't represent the real tool if the Multi-tool is being used.
      toolbar->SetCurrentTool((toolbar->GetDownTool()+(numTools-1))%numTools, true);
      mTrackPanel->Refresh(false);
   }
}


//
// Audio I/O Commands
//

// TODO: Should all these functions which involve
// the toolbar actually move into ControlToolBar?

void AudacityProject::OnStopSelect()
{
   wxCommandEvent evt;

   if (gAudioIO->IsStreamActive()) {
      mViewInfo.selectedRegion.setT0(gAudioIO->GetStreamTime(), false);
      GetControlToolBar()->OnStop(evt);
      ModifyState(false);           // without bWantsAutoSave
   }
}

void AudacityProject::OnCursorLongJumpLeft()
{
   OnCursorMove( false, true, true );
}

void AudacityProject::OnCursorLongJumpRight()
{
   OnCursorMove( true, true, true );
}

void AudacityProject::OnSelSetExtendLeft()
{
   OnBoundaryMove( true, false);
}

void AudacityProject::OnSelSetExtendRight()
{
   OnBoundaryMove( false, false);
}

void AudacityProject::OnSelExtendLeft(const wxEvent * evt)
{
   OnCursorLeft( true, false, evt->GetEventType() == wxEVT_KEY_UP );
}

void AudacityProject::OnSelExtendRight(const wxEvent * evt)
{
   OnCursorRight( true, false, evt->GetEventType() == wxEVT_KEY_UP );
}

void AudacityProject::OnSelContractLeft(const wxEvent * evt)
{
   OnCursorRight( true, true, evt->GetEventType() == wxEVT_KEY_UP );
}

void AudacityProject::OnSelContractRight(const wxEvent * evt)
{
   OnCursorLeft( true, true, evt->GetEventType() == wxEVT_KEY_UP );
}

//
// Effect Menus
//

/// OnEffect() takes a PluginID and has the EffectManager execute the assocated effect.
///
/// At the moment flags are used only to indicate whether to prompt for parameters and
/// whether to save the state to history.
bool AudacityProject::OnEffect(const PluginID & ID, int flags)
{
   const PluginDescriptor *plug = PluginManager::Get().GetPlugin(ID);
   wxASSERT(plug != NULL);
   EffectType type = plug->GetEffectType();

   // Make sure there's no activity since the effect is about to be applied
   // to the project's tracks.  Mainly for Apply during RTP, but also used
   // for batch commands
   if (flags & OnEffectFlagsConfigured)
   {
      TransportMenuCommands(this).OnStop();
      EditMenuCommands(this).SelectAllIfNone();
   }

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);

   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   WaveTrack *newTrack = NULL;
   wxWindow *focus = wxWindow::FindFocus();

   //double prevEndTime = mTracks->GetEndTime();
   int count = 0;
   bool clean = true;
   while (t) {
      if (t->GetSelected() && t->GetKind() == (Track::Wave)) {
         if (t->GetEndTime() != 0.0) clean = false;
         count++;
      }
      t = iter.Next();
   }

   if (count == 0) {
      // No tracks were selected...
      if (type == EffectTypeGenerate) {
         // Create a new track for the generated audio...
         newTrack = mTrackFactory->NewWaveTrack();
         mTracks->Add(newTrack);
         newTrack->SetSelected(true);
      }
   }

   EffectManager & em = EffectManager::Get();

   bool success = em.DoEffect(ID, this, mRate,
                               mTracks, mTrackFactory, 
                               &mViewInfo.selectedRegion,
                               (flags & OnEffectFlagsConfigured) == 0);

   if (!success) {
      if (newTrack) {
         mTracks->Remove(newTrack);
         delete newTrack;
         mTrackPanel->Refresh(false);
      }

      // For now, we're limiting realtime preview to a single effect, so
      // make sure the menus reflect that fact that one may have just been
      // opened.
      UpdateMenus(false);

      return false;
   }

   if (!(flags & OnEffectFlagsSkipState))
   {
      wxString shortDesc = em.GetEffectName(ID);
      wxString longDesc = em.GetEffectDescription(ID);
      PushState(longDesc, shortDesc);

      // Only remember a successful effect, don't rmemeber insert,
      // or analyze effects.
      if (type == EffectTypeProcess) {
         mLastEffect = ID;
         wxString lastEffectDesc;
         /* i18n-hint: %s will be the name of the effect which will be
            * repeated if this menu item is chosen */
         lastEffectDesc.Printf(_("Repeat %s"), shortDesc.c_str());
         GetCommandManager()->Modify(wxT("RepeatLastEffect"), lastEffectDesc);
      }
   }

   //STM:
   //The following automatically re-zooms after sound was generated.
   // IMO, it was disorienting, removing to try out without re-fitting
   //mchinen:12/14/08 reapplying for generate effects
   if (type == EffectTypeGenerate)
   {
      if (count == 0 || (clean && mViewInfo.selectedRegion.t0() == 0.0))
         ViewMenuCommands(this).OnZoomFit();
         //  mTrackPanel->Refresh(false);
   }
   RedrawProject();
   if (focus != NULL) {
      focus->SetFocus();
   }
   mTrackPanel->EnsureVisible(mTrackPanel->GetFirstSelectedTrack());

   mTrackPanel->Refresh(false);

   return true;
}

void AudacityProject::OnRepeatLastEffect(int WXUNUSED(index))
{
   if (!mLastEffect.IsEmpty())
   {
      OnEffect(mLastEffect, OnEffectFlagsConfigured);
   }
}




void AudacityProject::OnManagePluginsMenu(EffectType type)
{
   if (PluginManager::Get().ShowManager(this, type))
   {
      for (size_t i = 0; i < gAudacityProjects.GetCount(); i++) {
         AudacityProject *p = gAudacityProjects[i];

         p->RebuildMenuBar();
#if defined(__WXGTK__)
         // Workaround for:
         //
         //   http://bugzilla.audacityteam.org/show_bug.cgi?id=458
         //
         // This workaround should be removed when Audacity updates to wxWidgets 3.x which has a fix.
         wxRect r = p->GetRect();
         p->SetSize(wxSize(1,1));
         p->SetSize(r.GetSize());
#endif
      }
   }
}

void AudacityProject::OnManageGenerators()
{
   OnManagePluginsMenu(EffectTypeGenerate);
}

void AudacityProject::OnManageEffects()
{
   OnManagePluginsMenu(EffectTypeProcess);
}

void AudacityProject::OnManageAnalyzers()
{
   OnManagePluginsMenu(EffectTypeAnalyze);
}



//
// File Menu
//

void AudacityProject::OnNew()
{
   CreateNewAudacityProject();
}

void AudacityProject::OnOpen()
{
   OpenFiles(this);
}

void AudacityProject::OnClose()
{
   mMenuClose = true;
   Close();
}

void AudacityProject::OnSave()
{
   Save();
}

void AudacityProject::OnSaveAs()
{
   SaveAs();
}

#ifdef USE_LIBVORBIS
   void AudacityProject::OnSaveCompressed()
   {
      SaveAs(true);
   }
#endif

void AudacityProject::OnCheckDependencies()
{
   ShowDependencyDialogIfNeeded(this, false);
}

void AudacityProject::OnExit()
{
   QuitAudacity();
}

void AudacityProject::OnExportLabels()
{
   Track *t;
   int numLabelTracks = 0;

   TrackListIterator iter(mTracks);

   wxString fName = _("labels.txt");
   t = iter.First();
   while (t) {
      if (t->GetKind() == Track::Label)
      {
         numLabelTracks++;
         fName = t->GetName();
      }
      t = iter.Next();
   }

   if (numLabelTracks == 0) {
      wxMessageBox(_("There are no label tracks to export."));
      return;
   }

   fName = FileSelector(_("Export Labels As:"),
                        wxEmptyString,
                        fName,
                        wxT("txt"),
                        wxT("*.txt"),
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
                        this);

   if (fName == wxT(""))
      return;

   // Move existing files out of the way.  Otherwise wxTextFile will
   // append to (rather than replace) the current file.

   if (wxFileExists(fName)) {
#ifdef __WXGTK__
      wxString safetyFileName = fName + wxT("~");
#else
      wxString safetyFileName = fName + wxT(".bak");
#endif

      if (wxFileExists(safetyFileName))
         wxRemoveFile(safetyFileName);

      wxRename(fName, safetyFileName);
   }

   wxTextFile f(fName);
   f.Create();
   f.Open();
   if (!f.IsOpened()) {
      wxMessageBox(_("Couldn't write to file: ") + fName);
      return;
   }

   t = iter.First();
   while (t) {
      if (t->GetKind() == Track::Label)
         ((LabelTrack *) t)->Export(f);

      t = iter.Next();
   }

   f.Write();
   f.Close();
}


#ifdef USE_MIDI
void AudacityProject::OnExportMIDI(){
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   int numNoteTracksSelected = 0;
   NoteTrack *nt = NULL;

   // Iterate through once to make sure that there is
   // exactly one NoteTrack selected.
   while (t) {
      if (t->GetSelected()) {
         if(t->GetKind() == Track::Note) {
            numNoteTracksSelected++;
            nt = (NoteTrack *) t;
         }
      }
      t = iter.Next();
   }

   if(numNoteTracksSelected > 1){
      wxMessageBox(wxString::Format(wxT(
         "Please select only one MIDI track at a time.")));
      return;
   }

   wxASSERT(nt);
   if (!nt)
      return;

   while(true){

      wxString fName = wxT("");

      fName = FileSelector(_("Export MIDI As:"),
         wxEmptyString,
         fName,
         wxT(".mid|.gro"),
         _("MIDI file (*.mid)|*.mid|Allegro file (*.gro)|*.gro"),
         wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
         this);

      if (fName == wxT(""))
         return;

      if(!fName.Contains(wxT("."))) {
         fName = fName + wxT(".mid");
      }

      // Move existing files out of the way.  Otherwise wxTextFile will
      // append to (rather than replace) the current file.

      if (wxFileExists(fName)) {
#ifdef __WXGTK__
         wxString safetyFileName = fName + wxT("~");
#else
         wxString safetyFileName = fName + wxT(".bak");
#endif

         if (wxFileExists(safetyFileName))
            wxRemoveFile(safetyFileName);

         wxRename(fName, safetyFileName);
      }

      if(fName.EndsWith(wxT(".mid")) || fName.EndsWith(wxT(".midi"))) {
         nt->ExportMIDI(fName);
      } else if(fName.EndsWith(wxT(".gro"))) {
         nt->ExportAllegro(fName);
      } else {
         wxString msg = _("You have selected a filename with an unrecognized file extension.\nDo you want to continue?");
         wxString title = _("Export MIDI");
         int id = wxMessageBox(msg, title, wxYES_NO);
         if (id == wxNO) {
            continue;
         } else if (id == wxYES) {
            nt->ExportMIDI(fName);
         }
      }
      break;
   }
}
#endif // USE_MIDI


void AudacityProject::OnExport()
{
   Exporter e;

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);
   e.Process(this, false, 0.0, mTracks->GetEndTime());
}

void AudacityProject::OnExportSelection()
{
   Exporter e;

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);
   e.SetFileDialogTitle( _("Export Selected Audio") );
   e.Process(this, true, mViewInfo.selectedRegion.t0(),
      mViewInfo.selectedRegion.t1());
}

void AudacityProject::OnExportMultiple()
{
   ExportMultiple em(this);

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);
   em.ShowModal();
}

void AudacityProject::OnPageSetup()
{
   HandlePageSetup(this);
}

void AudacityProject::OnPrint()
{
   HandlePrint(this, GetName(), mTracks);
}

//
// View Menu
//

double AudacityProject::GetScreenEndTime() const
{
   return mTrackPanel->GetScreenEndTime();
}

void AudacityProject::ZoomInByFactor( double ZoomFactor )
{
   // LLL: Handling positioning differently when audio is active
   if (gAudioIO->IsStreamActive(GetAudioIOToken()) != 0) {
      ZoomBy(ZoomFactor);
      mTrackPanel->ScrollIntoView(gAudioIO->GetStreamTime());
      mTrackPanel->Refresh(false);
      return;
   }

   // DMM: Here's my attempt to get logical zooming behavior
   // when there's a selection that's currently at least
   // partially on-screen

   const double endTime = GetScreenEndTime();
   const double duration = endTime - mViewInfo.h;

   bool selectionIsOnscreen =
      (mViewInfo.selectedRegion.t0() < endTime) &&
      (mViewInfo.selectedRegion.t1() >= mViewInfo.h);

   bool selectionFillsScreen =
      (mViewInfo.selectedRegion.t0() < mViewInfo.h) &&
      (mViewInfo.selectedRegion.t1() > endTime);

   if (selectionIsOnscreen && !selectionFillsScreen) {
      // Start with the center of the selection
      double selCenter = (mViewInfo.selectedRegion.t0() +
                          mViewInfo.selectedRegion.t1()) / 2;

      // If the selection center is off-screen, pick the
      // center of the part that is on-screen.
      if (selCenter < mViewInfo.h)
         selCenter = mViewInfo.h +
                     (mViewInfo.selectedRegion.t1() - mViewInfo.h) / 2;
      if (selCenter > endTime)
         selCenter = endTime -
            (endTime - mViewInfo.selectedRegion.t0()) / 2;

      // Zoom in
      ZoomBy(ZoomFactor);
      const double newDuration = GetScreenEndTime() - mViewInfo.h;

      // Recenter on selCenter
      TP_ScrollWindow(selCenter - newDuration / 2);
      return;
   }


   double origLeft = mViewInfo.h;
   double origWidth = duration;
   ZoomBy(ZoomFactor);

   const double newDuration = GetScreenEndTime() - mViewInfo.h;
   double newh = origLeft + (origWidth - newDuration) / 2;

   // MM: Commented this out because it was confusing users
   /*
   // make sure that the *right-hand* end of the selection is
   // no further *left* than 1/3 of the way across the screen
   if (mViewInfo.selectedRegion.t1() < newh + mViewInfo.screen / 3)
      newh = mViewInfo.selectedRegion.t1() - mViewInfo.screen / 3;

   // make sure that the *left-hand* end of the selection is
   // no further *right* than 2/3 of the way across the screen
   if (mViewInfo.selectedRegion.t0() > newh + mViewInfo.screen * 2 / 3)
      newh = mViewInfo.selectedRegion.t0() - mViewInfo.screen * 2 / 3;
   */

   TP_ScrollWindow(newh);
}

void AudacityProject::ZoomOutByFactor( double ZoomFactor )
{
   //Zoom() may change these, so record original values:
   const double origLeft = mViewInfo.h;
   const double origWidth = GetScreenEndTime() - origLeft;

   ZoomBy(ZoomFactor);
   const double newWidth = GetScreenEndTime() - mViewInfo.h;

   const double newh = origLeft + (origWidth - newWidth) / 2;
   // newh = (newh > 0) ? newh : 0;
   TP_ScrollWindow(newh);

}

// this is unused:
#if 0
static double OldZooms[2]={ 44100.0/512.0, 4410.0/512.0 };
void AudacityProject::OnZoomToggle()
{
   double origLeft = mViewInfo.h;
   double origWidth = mViewInfo.screen;

   float f;
   // look at percentage difference.  We add a small fudge factor
   // to avoid testing for zero divisor.
   f = mViewInfo.zoom / (OldZooms[0] + 0.0001f);
   // If old zoom is more than 10 percent different, use it.
   if( (0.90f > f) || (f >1.10) ){
      OldZooms[1]=OldZooms[0];
      OldZooms[0]=mViewInfo.zoom;
   }
   Zoom( OldZooms[1] );
   double newh = origLeft + (origWidth - mViewInfo.screen) / 2;
   TP_ScrollWindow(newh);
}
#endif


void AudacityProject::DoZoomFitV()
{
   int height, count;

   mTrackPanel->GetTracksUsableArea(NULL, &height);

   height -= 28;

   count = 0;
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   while (t) {
      if ((t->GetKind() == Track::Wave) &&
          !t->GetMinimized())
         count++;
      else
         height -= t->GetHeight();

      t = iter.Next();
   }

   if (count == 0)
      return;

   height = height / count;

   if (height < 40)
      height = 40;

   TrackListIterator iter2(mTracks);
   t = iter2.First();
   while (t) {
      if ((t->GetKind() == Track::Wave) &&
          !t->GetMinimized())
         t->SetHeight(height);
      t = iter2.Next();
   }
}

void AudacityProject::OnPlotSpectrum()
{
   if (!mFreqWindow) {
      wxPoint where;

      where.x = 150;
      where.y = 150;

      mFreqWindow = new FreqWindow(this, -1, _("Frequency Analysis"), where);
   }

   mFreqWindow->Show(true);
   mFreqWindow->Raise();
   mFreqWindow->SetFocus();
}

void AudacityProject::OnContrast()
{
   // All of this goes away when the Contrast Dialog is converted to a module
   if(!mContrastDialog)
   {
      wxPoint where;

      where.x = 150;
      where.y = 150;

      mContrastDialog = new ContrastDialog(this, -1, _("Contrast Analysis (WCAG 2 compliance)"), where);

      mContrastDialog->bFGset = false;
      mContrastDialog->bBGset = false;
   }

   // Zero dialog boxes.  Do we need to do this here?
   if( !mContrastDialog->bFGset )
   {
      mContrastDialog->mForegroundStartT->SetValue(0.0);
      mContrastDialog->mForegroundEndT->SetValue(0.0);
   }
   if( !mContrastDialog->bBGset )
   {
      mContrastDialog->mBackgroundStartT->SetValue(0.0);
      mContrastDialog->mBackgroundEndT->SetValue(0.0);
   }

   mContrastDialog->CentreOnParent();
   mContrastDialog->Show();
}


//
// Project Menu
//

void AudacityProject::OnImport()
{
   // An import trigger for the alias missing dialog might not be intuitive, but
   // this serves to track the file if the users zooms in and such.
   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);

   wxArrayString selectedFiles = ShowOpenDialog(wxT(""));
   if (selectedFiles.GetCount() == 0) {
      gPrefs->Write(wxT("/LastOpenType"),wxT(""));
      gPrefs->Flush();
      return;
   }

   gPrefs->Write(wxT("/NewImportingSession"), true);

   //sort selected files by OD status.  Load non OD first so user can edit asap.
   //first sort selectedFiles.
   selectedFiles.Sort(CompareNoCaseFileName);
   ODManager::Pause();

   for (size_t ff = 0; ff < selectedFiles.GetCount(); ff++) {
      wxString fileName = selectedFiles[ff];

      wxString path = ::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);

      Import(fileName);
   }

   gPrefs->Write(wxT("/LastOpenType"),wxT(""));

   gPrefs->Flush();

   HandleResize(); // Adjust scrollers for new track sizes.
   ODManager::Resume();
}

void AudacityProject::OnImportLabels()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"),::wxGetCwd());

   wxString fileName =
       FileSelector(_("Select a text file containing labels..."),
                    path,     // Path
                    wxT(""),       // Name
                    wxT(".txt"),   // Extension
                    _("Text files (*.txt)|*.txt|All files|*"),
                    wxRESIZE_BORDER,        // Flags
                    this);    // Parent

   if (fileName != wxT("")) {
      path =::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);
      gPrefs->Flush();

      wxTextFile f;

      f.Open(fileName);
      if (!f.IsOpened()) {
         wxMessageBox(_("Could not open file: ") + fileName);
         return;
      }

      LabelTrack *newTrack = new LabelTrack(mDirManager);
      wxString sTrackName;
      wxFileName::SplitPath(fileName, NULL, NULL, &sTrackName, NULL);
      newTrack->SetName(sTrackName);

      newTrack->Import(f);

      SelectNone();
      mTracks->Add(newTrack);
      newTrack->SetSelected(true);

      PushState(wxString::
                Format(_("Imported labels from '%s'"), fileName.c_str()),
                _("Import Labels"));

      RedrawProject();
   }
}

#ifdef USE_MIDI
void AudacityProject::OnImportMIDI()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"),::wxGetCwd());

   wxString fileName = FileSelector(_("Select a MIDI file..."),
                                    path,     // Path
                                    wxT(""),       // Name
                                    wxT(""),       // Extension
                                    _("MIDI and Allegro files (*.mid;*.midi;*.gro)|*.mid;*.midi;*.gro|MIDI files (*.mid;*.midi)|*.mid;*.midi|Allegro files (*.gro)|*.gro|All files|*"),
                                    wxRESIZE_BORDER,        // Flags
                                    this);    // Parent

   if (fileName != wxT("")) {
      path =::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);
      gPrefs->Flush();

      NoteTrack *newTrack = new NoteTrack(mDirManager);

      if (::ImportMIDI(fileName, newTrack)) {

         SelectNone();
         mTracks->Add(newTrack);
         newTrack->SetSelected(true);

         PushState(wxString::Format(_("Imported MIDI from '%s'"),
                                    fileName.c_str()), _("Import MIDI"));

         RedrawProject();
         mTrackPanel->EnsureVisible(newTrack);
      }
   }
}
#endif // USE_MIDI

void AudacityProject::OnImportRaw()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"),::wxGetCwd());

   wxString fileName =
       FileSelector(_("Select any uncompressed audio file..."),
                    path,     // Path
                    wxT(""),       // Name
                    wxT(""),       // Extension
                    _("All files|*"),
                    wxRESIZE_BORDER,        // Flags
                    this);    // Parent

   if (fileName == wxT(""))
      return;

   path =::wxPathOnly(fileName);
   gPrefs->Write(wxT("/DefaultOpenPath"), path);
   gPrefs->Flush();

   Track **newTracks;
   int numTracks;

   numTracks = ::ImportRaw(this, fileName, mTrackFactory, &newTracks);

   if (numTracks <= 0)
      return;

   AddImportedTracks(fileName, newTracks, numTracks);
   HandleResize(); // Adjust scrollers for new track sizes.
}

void AudacityProject::OnEditMetadata()
{
   if (mTags->ShowEditDialog(this, _("Edit Metadata Tags"), true))
      PushState(_("Edit Metadata Tags"), _("Edit Metadata"));
}

void AudacityProject::OnApplyChain()
{
   BatchProcessDialog dlg(this);
   dlg.ShowModal();
   ModifyUndoMenuItems();
}

void AudacityProject::OnEditChains()
{
   EditChainsDialog dlg(this);
   dlg.ShowModal();
}

//
// Help Menu
//

void AudacityProject::OnHelpWelcome()
{
   SplashDialog::Show2( this );
}

void AudacityProject::OnSeparator()
{

}

void AudacityProject::OnSnapToOff()
{
   SetSnapTo(SNAP_OFF);
}

void AudacityProject::OnSnapToNearest()
{
   SetSnapTo(SNAP_NEAREST);
}

void AudacityProject::OnSnapToPrior()
{
   SetSnapTo(SNAP_PRIOR);
}

void AudacityProject::OnCursorLeft(bool shift, bool ctrl, bool keyup)
{
   // PRL:  What I found and preserved, strange though it be:
   // During playback:  jump depends on preferences and is independent of the zoom
   // and does not vary if the key is held
   // Else: jump depends on the zoom and gets bigger if the key is held
   int snapToTime = GetSnapTo();
   double quietSeekStepPositive = 1.0; // pixels
   double audioSeekStepPositive = shift ? mSeekLong : mSeekShort;
   SeekLeftOrRight
      (true, shift, ctrl, keyup, snapToTime, true, false,
       quietSeekStepPositive, true,
       audioSeekStepPositive, false);
}

void AudacityProject::OnCursorRight(bool shift, bool ctrl, bool keyup)
{
   // PRL:  What I found and preserved, strange though it be:
   // During playback:  jump depends on preferences and is independent of the zoom
   // and does not vary if the key is held
   // Else: jump depends on the zoom and gets bigger if the key is held
   int snapToTime = GetSnapTo();
   double quietSeekStepPositive = 1.0; // pixels
   double audioSeekStepPositive = shift ? mSeekLong : mSeekShort;
   SeekLeftOrRight
      (false, shift, ctrl, keyup, snapToTime, true, false,
       quietSeekStepPositive, true,
       audioSeekStepPositive, false);
}

// Handle small cursor and play head movements
void AudacityProject::SeekLeftOrRight
(bool leftward, bool shift, bool ctrl, bool keyup,
 int snapToTime, bool mayAccelerateQuiet, bool mayAccelerateAudio,
 double quietSeekStepPositive, bool quietStepIsPixels,
 double audioSeekStepPositive, bool audioStepIsPixels)
{
   if (keyup)
   {
      if (IsAudioActive())
      {
         return;
      }

      ModifyState(false);
      return;
   }

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   const wxLongLong curtime = ::wxGetLocalTimeMillis();
   enum { MIN_INTERVAL = 50 };
   const bool fast = (curtime - mLastSelectionAdjustment < MIN_INTERVAL);

   // How much faster should the cursor move if shift is down?
   enum { LARGER_MULTIPLIER = 4 };
   int multiplier = (fast && mayAccelerateQuiet) ? LARGER_MULTIPLIER : 1;
   if (leftward)
      multiplier = -multiplier;

   if (shift && ctrl)
   {
      mLastSelectionAdjustment = curtime;

      // Contract selection
      // Reduce and constrain (counter-intuitive)
      if (leftward) {
         const double t1 = mViewInfo.selectedRegion.t1();
         mViewInfo.selectedRegion.setT1(
            std::max(mViewInfo.selectedRegion.t0(),
               snapToTime
               ? GridMove(t1, multiplier)
               : quietStepIsPixels
                  ? mViewInfo.OffsetTimeByPixels(
                        t1, int(multiplier * quietSeekStepPositive))
                  : t1 +  multiplier * quietSeekStepPositive
         ));

         // Make sure it's visible.
         GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t1());
      }
      else {
         const double t0 = mViewInfo.selectedRegion.t0();
         mViewInfo.selectedRegion.setT0(
            std::min(mViewInfo.selectedRegion.t1(),
               snapToTime
               ? GridMove(t0, multiplier)
               : quietStepIsPixels
                  ? mViewInfo.OffsetTimeByPixels(
                     t0, int(multiplier * quietSeekStepPositive))
                  : t0 + multiplier * quietSeekStepPositive
         ));

         // Make sure new position is in view.
         GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t0());
      }
      GetTrackPanel()->Refresh(false);
   }
   else if (IsAudioActive()) {
#ifdef EXPERIMENTAL_IMPROVED_SEEKING
      if (gAudioIO->GetLastPlaybackTime() < mLastSelectionAdjustment) {
         // Allow time for the last seek to output a buffer before
         // discarding samples again
         // Do not advance mLastSelectionAdjustment
         return;
      }
#endif
      mLastSelectionAdjustment = curtime;

      // Ignore the multiplier for the quiet case
      multiplier = (fast && mayAccelerateAudio) ? LARGER_MULTIPLIER : 1;
      if (leftward)
         multiplier = -multiplier;

      // If playing, reposition
      double seconds;
      if (audioStepIsPixels) {
         const double streamTime = gAudioIO->GetStreamTime();
         const double newTime =
            mViewInfo.OffsetTimeByPixels(streamTime, int(audioSeekStepPositive));
         seconds = newTime - streamTime;
      }
      else
         seconds = multiplier * audioSeekStepPositive;
      gAudioIO->SeekStream(seconds);
      return;
   }
   else if (shift)
   {
      mLastSelectionAdjustment = curtime;

      // Extend selection
      // Expand and constrain
      if (leftward) {
         const double t0 = mViewInfo.selectedRegion.t0();
         mViewInfo.selectedRegion.setT0(
            std::max(0.0,
               snapToTime
               ? GridMove(t0, multiplier)
               : quietStepIsPixels
                  ? mViewInfo.OffsetTimeByPixels(
                        t0, int(multiplier * quietSeekStepPositive))
                  : t0 + multiplier * quietSeekStepPositive
         ));

         // Make sure it's visible.
         GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t0());
      }
      else {
         const double end = mTracks->GetEndTime();
         const double t1 = mViewInfo.selectedRegion.t1();
         mViewInfo.selectedRegion.setT1(
            std::min(end,
               snapToTime
               ? GridMove(t1, multiplier)
               : quietStepIsPixels
                  ? mViewInfo.OffsetTimeByPixels(
                        t1, int(multiplier * quietSeekStepPositive))
                  : t1 + multiplier * quietSeekStepPositive
         ));

         // Make sure new position is in view.
         GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t1());
      }
      GetTrackPanel()->Refresh(false);
   }
   else
   {
      mLastSelectionAdjustment = curtime;

      // Move the cursor
      // Already in cursor mode?
      if (mViewInfo.selectedRegion.isPoint())
      {
         // Move and constrain
         const double end = mTracks->GetEndTime();
         const double t0 = mViewInfo.selectedRegion.t0();
         mViewInfo.selectedRegion.setT0(
            std::max(0.0,
               std::min(end,
                  snapToTime
                  ? GridMove(t0, multiplier)
                  : quietStepIsPixels
                     ? mViewInfo.OffsetTimeByPixels(
                          t0, int(multiplier * quietSeekStepPositive))
                     : t0 + multiplier * quietSeekStepPositive)),
            false // do not swap selection boundaries
         );
         mViewInfo.selectedRegion.collapseToT0();

         // Move the visual cursor, avoiding an unnecessary complete redraw
         GetTrackPanel()->DrawOverlays(false);
      }
      else
      {
         // Transition to cursor mode.
         if (leftward)
            mViewInfo.selectedRegion.collapseToT0();
         else
            mViewInfo.selectedRegion.collapseToT1();
         GetTrackPanel()->Refresh(false);
      }

      // Make sure new position is in view
      GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t1());
   }
}

// Handles moving a selection edge with the keyboard in snap-to-time mode;
// returns the moved value.
// Will move at least minPix pixels -- set minPix positive to move forward,
// negative to move backward.
double AudacityProject::GridMove(double t, int minPix)
{
   NumericConverter nc(NumericConverter::TIME, GetSelectionFormat(), t, GetRate());

   // Try incrementing/decrementing the value; if we've moved far enough we're
   // done
   double result;
   minPix >= 0 ? nc.Increment() : nc.Decrement();
   result = nc.GetValue();
   if (std::abs(mViewInfo.TimeToPosition(result) - mViewInfo.TimeToPosition(t))
       >= abs(minPix))
       return result;

   // Otherwise, move minPix pixels, then snap to the time.
   result = mViewInfo.OffsetTimeByPixels(t, minPix);
   nc.SetValue(result);
   result = nc.GetValue();
   return result;
}

void AudacityProject::OnBoundaryMove(bool left, bool boundaryContract)
{
  // Move the left/right selection boundary, to either expand or contract the selection
  // left=true: operate on left boundary; left=false: operate on right boundary
  // boundaryContract=true: contract region; boundaryContract=false: expand region.

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   wxLongLong curtime = ::wxGetLocalTimeMillis();
   int pixels = 1;
   if( curtime - mLastSelectionAdjustment < 50 )
   {
      pixels = 4;
   }
   mLastSelectionAdjustment = curtime;

   if (IsAudioActive())
   {
      double indicator = gAudioIO->GetStreamTime();
      if (left)
         mViewInfo.selectedRegion.setT0(indicator, false);
      else
         mViewInfo.selectedRegion.setT1(indicator);

      ModifyState(false);
      GetTrackPanel()->Refresh(false);
   }
   else
   {
      // BOUNDARY MOVEMENT
      // Contract selection from the right to the left
      if( boundaryContract )
      {
         if (left) {
            // Reduce and constrain left boundary (counter-intuitive)
            // Move the left boundary by at most the desired number of pixels,
            // but not past the right
            mViewInfo.selectedRegion.setT0(
               std::min(mViewInfo.selectedRegion.t1(),
                  mViewInfo.OffsetTimeByPixels(
                     mViewInfo.selectedRegion.t0(),
                     pixels)));

            // Make sure it's visible
            GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t0());
         }
         else
         {
            // Reduce and constrain right boundary (counter-intuitive)
            // Move the right boundary by at most the desired number of pixels,
            // but not past the left
            mViewInfo.selectedRegion.setT1(
               std::max(mViewInfo.selectedRegion.t0(),
                  mViewInfo.OffsetTimeByPixels(
                     mViewInfo.selectedRegion.t1(),
                     -pixels)));

            // Make sure it's visible
            GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t1());
         }
      }
      // BOUNDARY MOVEMENT
      // Extend selection toward the left
      else
      {
         if (left) {
            // Expand and constrain left boundary
            mViewInfo.selectedRegion.setT0(
               std::max(0.0,
                  mViewInfo.OffsetTimeByPixels(
                     mViewInfo.selectedRegion.t0(),
                     -pixels)));

            // Make sure it's visible
            GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t0());
         }
         else
         {
            // Expand and constrain right boundary
            const double end = mTracks->GetEndTime();
            mViewInfo.selectedRegion.setT1(
               std::min(end,
                  mViewInfo.OffsetTimeByPixels(
                     mViewInfo.selectedRegion.t1(),
                     pixels)));

            // Make sure it's visible
            GetTrackPanel()->ScrollIntoView(mViewInfo.selectedRegion.t1());
         }
      }
      GetTrackPanel()->Refresh( false );
      ModifyState(false);
   }
}

// Move the cursor forward or backward, while paused or while playing.
// forward=true: Move cursor forward; forward=false: Move cursor backwards
// jump=false: Move cursor determined by zoom; jump=true: Use seek times
// longjump=false: Use mSeekShort; longjump=true: Use mSeekLong
void AudacityProject::OnCursorMove(bool forward, bool jump, bool longjump )
{
   // PRL:  nobody calls this yet with !jump

   double positiveSeekStep;
   bool byPixels;
   if (jump) {
      if (!longjump) {
         positiveSeekStep = mSeekShort;
      } else {
         positiveSeekStep = mSeekLong;
      }
      byPixels = false;
   } else {
      positiveSeekStep = 1.0;
      byPixels = true;
   }
   bool mayAccelerate = !jump;
   SeekLeftOrRight
      (!forward, false, false, false,
       0, mayAccelerate, mayAccelerate,
       positiveSeekStep, byPixels,
       positiveSeekStep, byPixels);

   ModifyState(false);
}
