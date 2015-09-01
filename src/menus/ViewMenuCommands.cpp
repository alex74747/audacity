#include "../Audacity.h"
#include "../Experimental.h"
#include "ViewMenuCommands.h"

#include <algorithm>

#include <wx/scrolbar.h>

#include "../HistoryWindow.h"
#include "../LyricsWindow.h"
#include "../MixerBoard.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../TrackPanel.h"
#include "../commands/CommandManager.h"
#include "../toolbars/ToolManager.h"

#define FN(X) FNT(ViewMenuCommands, this, & ViewMenuCommands :: X)

ViewMenuCommands::ViewMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void ViewMenuCommands::Create(CommandManager *c)
{
   c->BeginMenu(_("&View"));
   {
      c->SetDefaultFlags(TracksExistFlag, TracksExistFlag);

      c->AddItem(wxT("ZoomIn"), _("Zoom &In"), FN(OnZoomIn), wxT("Ctrl+1"),
         ZoomInAvailableFlag,
         ZoomInAvailableFlag);
      c->AddItem(wxT("ZoomNormal"), _("Zoom &Normal"), FN(OnZoomNormal), wxT("Ctrl+2"));
      c->AddItem(wxT("ZoomOut"), _("Zoom &Out"), FN(OnZoomOut), wxT("Ctrl+3"),
         ZoomOutAvailableFlag,
         ZoomOutAvailableFlag);
      c->AddItem(wxT("ZoomSel"), _("&Zoom to Selection"), FN(OnZoomSel), wxT("Ctrl+E"), TimeSelectedFlag, TimeSelectedFlag);

      c->AddSeparator();
      c->AddItem(wxT("FitInWindow"), _("&Fit in Window"), FN(OnZoomFit), wxT("Ctrl+F"));
      c->AddItem(wxT("FitV"), _("Fit &Vertically"), FN(OnZoomFitV), wxT("Ctrl+Shift+F"));

      c->AddSeparator();
      c->AddItem(wxT("GoSelStart"), _("Go to Selection Sta&rt"), FN(OnGoSelStart), wxT("Ctrl+["), TimeSelectedFlag, TimeSelectedFlag);
      c->AddItem(wxT("GoSelEnd"), _("Go to Selection En&d"), FN(OnGoSelEnd), wxT("Ctrl+]"), TimeSelectedFlag, TimeSelectedFlag);

      c->AddSeparator();
      c->AddItem(wxT("CollapseAllTracks"), _("&Collapse All Tracks"), FN(OnCollapseAllTracks), wxT("Ctrl+Shift+C"));

      c->AddItem(wxT("ExpandAllTracks"), _("E&xpand Collapsed Tracks"), FN(OnExpandAllTracks), wxT("Ctrl+Shift+X"));

      c->AddSeparator();
      c->AddCheck(wxT("ShowClipping"), _("&Show Clipping"), FN(OnShowClipping),
         gPrefs->Read(wxT("/GUI/ShowClipping"), 0L), AlwaysEnabledFlag, AlwaysEnabledFlag);

      //c->AddSeparator();

      // History window should be available either for UndoAvailableFlag or RedoAvailableFlag,
      // but we can't make the AddItem flags and mask have both, because they'd both have to be true for the
      // command to be enabled.
      //    If user has Undone the entire stack, RedoAvailableFlag is on but UndoAvailableFlag is off.
      //    If user has done things but not Undone anything, RedoAvailableFlag is off but UndoAvailableFlag is on.
      // So in either of those cases, (AudioIONotBusyFlag | UndoAvailableFlag | RedoAvailableFlag) mask
      // would fail.
      // The only way to fix this in the current architecture is to hack in special cases for RedoAvailableFlag
      // in AudacityProject::UpdateMenus() (ugly) and CommandManager::HandleCommandEntry() (*really* ugly --
      // shouldn't know about particular command names and flags).
      // Here's the hack that would be necessary in AudacityProject::UpdateMenus(), if somebody decides to do it:
      //    // Because EnableUsingFlags requires all the flag bits match the corresponding mask bits,
      //    // "UndoHistory" specifies only AudioIONotBusyFlag | UndoAvailableFlag, because that
      //    // covers the majority of cases where it should be enabled.
      //    // If history is not empty but we've Undone the whole stack, we also want to enable,
      //    // to show the Redo's on stack.
      //    // "UndoHistory" might already be enabled, but add this check for RedoAvailableFlag.
      //    if (flags & RedoAvailableFlag)
      //       GetCommandManager()->Enable(wxT("UndoHistory"), true);
      // So for now, enable the command regardless of stack. It will just show empty sometimes.
      // FOR REDESIGN, clearly there are some limitations with the flags/mask bitmaps.

      /* i18n-hint: Clicking this menu item shows the various editing steps that have been taken.*/
      c->AddItem(wxT("UndoHistory"), _("&History..."), FN(OnHistory),
         AudioIONotBusyFlag,
         AudioIONotBusyFlag);
      c->AddItem(wxT("Karaoke"), _("&Karaoke..."), FN(OnKaraoke), LabelTracksExistFlag, LabelTracksExistFlag);
      c->AddItem(wxT("MixerBoard"), _("&Mixer Board..."), FN(OnMixerBoard), WaveTracksExistFlag, WaveTracksExistFlag);

      c->AddSeparator();

      /////////////////////////////////////////////////////////////////////////////

      c->BeginSubMenu(_("&Toolbars"));
      {
         /* i18n-hint: Clicking this menu item shows the toolbar that manages devices*/
            c->AddCheck(wxT("ShowDeviceTB"), _("&Device Toolbar"), FN(OnShowDeviceToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar for editing*/
            c->AddCheck(wxT("ShowEditTB"), _("&Edit Toolbar"), FN(OnShowEditToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar which has sound level meters*/
            c->AddCheck(wxT("ShowMeterTB"), _("&Combined Meter Toolbar"), FN(OnShowMeterToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar with the recording level meters*/
            c->AddCheck(wxT("ShowRecordMeterTB"), _("&Recording Meter Toolbar"), FN(OnShowRecordMeterToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar with the playback level meter*/
            c->AddCheck(wxT("ShowPlayMeterTB"), _("&Playback Meter Toolbar"), FN(OnShowPlayMeterToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar with the mixer*/
            c->AddCheck(wxT("ShowMixerTB"), _("Mi&xer Toolbar"), FN(OnShowMixerToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar for selecting a time range of audio*/
            c->AddCheck(wxT("ShowSelectionTB"), _("&Selection Toolbar"), FN(OnShowSelectionToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
      #ifdef EXPERIMENTAL_SPECTRAL_EDITING
            /* i18n-hint: Clicking this menu item shows the toolbar for selecting a frequency range of audio*/
            c->AddCheck(wxT("ShowSpectralSelectionTB"), _("&Spectral Selection Toolbar"), FN(OnShowSpectralSelectionToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
      #endif
            /* i18n-hint: Clicking this menu item shows a toolbar that has some tools in it*/
            c->AddCheck(wxT("ShowToolsTB"), _("T&ools Toolbar"), FN(OnShowToolsToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar for transcription (currently just vary play speed)*/
            c->AddCheck(wxT("ShowTranscriptionTB"), _("Transcri&ption Toolbar"), FN(OnShowTranscriptionToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar with the big buttons on it (play record etc)*/
            c->AddCheck(wxT("ShowTransportTB"), _("&Transport Toolbar"), FN(OnShowTransportToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
            /* i18n-hint: Clicking this menu item shows the toolbar that enables Scrub or Seek playback and Scrub Ruler*/
            c->AddCheck(wxT("ShowScrubbingTB"), _("Scru&b Toolbar"), FN(OnShowScrubbingToolBar), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);

         c->AddSeparator();

         /* i18n-hint: (verb)*/
         c->AddItem(wxT("ResetToolbars"), _("Reset Toolb&ars"), FN(OnResetToolBars), 0, AlwaysEnabledFlag, AlwaysEnabledFlag);
      }
      c->EndSubMenu();
   }
   c->EndMenu();
}

void ViewMenuCommands::CreateNonMenuCommands(CommandManager *c)
{
   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddCommand(wxT("FullScreenOnOff"), _("Full screen on/off"), FN(OnFullScreen),
#ifdef __WXMAC__
      wxT("Ctrl+/"));
#else
      wxT("F11"));
#endif
}

void ViewMenuCommands::OnZoomIn()
{
   mProject->ZoomInByFactor(2.0);
}

void ViewMenuCommands::OnZoomNormal()
{
   mProject->Zoom(ZoomInfo::GetDefaultZoom());
   mProject->GetTrackPanel()->Refresh(false);
}

void ViewMenuCommands::OnZoomOut()
{
   mProject->ZoomOutByFactor(1 / 2.0);
}

void ViewMenuCommands::OnZoomSel()
{
   const double lowerBound =
      std::max(mProject->GetViewInfo().selectedRegion.t0(), mProject->ScrollingLowerBoundTime());
   const double denom =
      mProject->GetViewInfo().selectedRegion.t1() - lowerBound;
   if (denom <= 0.0)
      return;

   // LL:  The "-1" is just a hack to get around an issue where zooming to
   //      selection doesn't actually get the entire selected region within the
   //      visible area.  This causes a problem with scrolling at end of playback
   //      where the selected region may be scrolled off the left of the screen.
   //      I know this isn't right, but until the real rounding or 1-off issue is
   //      found, this will have to work.
   // PRL:  Did I fix this?  I am not sure, so I leave the hack in place.
   //      Fixes might have resulted from commits
   //      1b8f44d0537d987c59653b11ed75a842b48896ea and
   //      e7c7bb84a966c3b3cc4b3a9717d5f247f25e7296
   int width;
   mProject->GetTrackPanel()->GetTracksUsableArea(&width, NULL);
   mProject->Zoom((width - 1) / denom);
   mProject->TP_ScrollWindow(mProject->GetViewInfo().selectedRegion.t0());
}

void ViewMenuCommands::OnZoomFit()
{
   const double end = mProject->GetTracks()->GetEndTime();
   const double start = mProject->GetViewInfo().bScrollBeyondZero
      ? std::min(mProject->GetTracks()->GetStartTime(), 0.0)
      : 0;
   const double len = end - start;

   if (len <= 0.0)
      return;

   int w;
   mProject->GetTrackPanel()->GetTracksUsableArea(&w, NULL);
   w -= 10;

   mProject->Zoom(w / len);
   mProject->TP_ScrollWindow(start);
}

void ViewMenuCommands::OnZoomFitV()
{
   mProject->DoZoomFitV();

   mProject->GetVerticalScrollBar()->SetThumbPosition(0);
   mProject->RedrawProject();
   mProject->ModifyState(true);
}

void ViewMenuCommands::OnGoSelStart()
{
   const ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   mProject->TP_ScrollWindow(viewInfo.selectedRegion.t0() -
      ((mProject->GetScreenEndTime() - viewInfo.h) / 2));
}

void ViewMenuCommands::OnGoSelEnd()
{
   const ViewInfo &viewInfo = mProject->GetViewInfo();
   if (viewInfo.selectedRegion.isPoint())
      return;

   mProject->TP_ScrollWindow(viewInfo.selectedRegion.t1() -
      ((mProject->GetScreenEndTime() - viewInfo.h) / 2));
}

void ViewMenuCommands::OnCollapseAllTracks()
{
   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();

   while (t)
   {
      t->SetMinimized(true);
      t = iter.Next();
   }

   mProject->ModifyState(true);
   mProject->RedrawProject();
}

void ViewMenuCommands::OnExpandAllTracks()
{
   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();

   while (t)
   {
      t->SetMinimized(false);
      t = iter.Next();
   }

   mProject->ModifyState(true);
   mProject->RedrawProject();
}

void ViewMenuCommands::OnShowClipping()
{
   bool checked = !gPrefs->Read(wxT("/GUI/ShowClipping"), 0L);
   gPrefs->Write(wxT("/GUI/ShowClipping"), checked);
   gPrefs->Flush();
   mProject->GetCommandManager()->Check(wxT("ShowClipping"), checked);
   mProject->GetTrackPanel()->UpdatePrefs();
   mProject->GetTrackPanel()->Refresh(false);
}

void ViewMenuCommands::OnHistory()
{
   HistoryWindow *historyWindow = mProject->GetHistoryWindow(true);

   historyWindow->Show();
   historyWindow->Raise();
   historyWindow->UpdateDisplay();
}

void ViewMenuCommands::OnKaraoke()
{
   LyricsWindow *lyricsWindow = mProject->GetLyricsWindow();

   lyricsWindow->Show();
   mProject->UpdateLyrics();
   lyricsWindow->Raise();
}

void ViewMenuCommands::OnMixerBoard()
{
   MixerBoardFrame *mixerBoardFrame = mProject->GetMixerBoardFrame();
   mixerBoardFrame->Show();
   mixerBoardFrame->Raise();
   mixerBoardFrame->SetFocus();
}

void ViewMenuCommands::OnShowDeviceToolBar()
{
   mProject->GetToolManager()->ShowHide(DeviceBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowEditToolBar()
{
   mProject->GetToolManager()->ShowHide(EditBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowMeterToolBar()
{
   if (!mProject->GetToolManager()->IsVisible(MeterBarID))
   {
      mProject->GetToolManager()->Expose(PlayMeterBarID, false);
      mProject->GetToolManager()->Expose(RecordMeterBarID, false);
   }
   mProject->GetToolManager()->ShowHide(MeterBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowRecordMeterToolBar()
{
   if (!mProject->GetToolManager()->IsVisible(RecordMeterBarID))
   {
      mProject->GetToolManager()->Expose(MeterBarID, false);
   }
   mProject->GetToolManager()->ShowHide(RecordMeterBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowPlayMeterToolBar()
{
   if (!mProject->GetToolManager()->IsVisible(PlayMeterBarID))
   {
      mProject->GetToolManager()->Expose(MeterBarID, false);
   }
   mProject->GetToolManager()->ShowHide(PlayMeterBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowMixerToolBar()
{
   mProject->GetToolManager()->ShowHide(MixerBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowSelectionToolBar()
{
   mProject->GetToolManager()->ShowHide(SelectionBarID);
   mProject->ModifyToolbarMenus();
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
void ViewMenuCommands::OnShowSpectralSelectionToolBar()
{
   mProject->GetToolManager()->ShowHide(SpectralSelectionBarID);
   mProject->ModifyToolbarMenus();
}
#endif

void ViewMenuCommands::OnShowToolsToolBar()
{
   mProject->GetToolManager()->ShowHide(ToolsBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowTranscriptionToolBar()
{
   mProject->GetToolManager()->ShowHide(TranscriptionBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowTransportToolBar()
{
   mProject->GetToolManager()->ShowHide(TransportBarID);
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnShowScrubbingToolBar()
{
   mProject->GetToolManager()->ShowHide( ScrubbingBarID );
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnResetToolBars()
{
   mProject->GetToolManager()->Reset();
   mProject->ModifyToolbarMenus();
}

void ViewMenuCommands::OnFullScreen()
{
   if (mProject->wxTopLevelWindow::IsFullScreen())
      mProject->wxTopLevelWindow::ShowFullScreen(false);
   else
      mProject->wxTopLevelWindow::ShowFullScreen(true);
}
