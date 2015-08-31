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

#define FN(X) new ObjectCommandFunctor<ViewMenuCommands>(this, &ViewMenuCommands:: X )

ViewMenuCommands::ViewMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void ViewMenuCommands::Create(CommandManager *c)
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

   c->AddItem(wxT("ExpandAllTracks"), _("E&xpand All Tracks"), FN(OnExpandAllTracks), wxT("Ctrl+Shift+X"));

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
   HistoryWindow *historyWindow = mProject->GetHistoryWindow();

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
