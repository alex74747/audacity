/**********************************************************************

Audacity: A Digital Audio Editor

TrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../Audacity.h"
#include "TrackControls.h"
#include "TrackButtonHandles.h"
#include "TrackSelectHandle.h"
#include "../../AColor.h"
#include "../../HitTestResult.h"
#include "../../RefreshCode.h"
#include "../../MixerBoard.h"
#include "../../Project.h"
#include "../../ShuttleGui.h"
#include "../../TrackPanel.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../WaveTrack.h"
#include <wx/textdlg.h>

int TrackControls::gCaptureState;

TrackControls::~TrackControls()
{
}

HitTestResult TrackControls::HitTest
(const TrackPanelMouseEvent &evt,
 const AudacityProject *project)
{
   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;
   HitTestResult result;

   if (NULL != (result = CloseButtonHandle::HitTest(event, rect, this)).handle)
      return result;

   if (NULL != (result = MenuButtonHandle::HitTest(event, rect, this)).handle)
      return result;

   if (NULL != (result = MinimizeButtonHandle::HitTest(event, rect)).handle)
      return result;

   return TrackSelectHandle::HitAnywhere
      (project->GetTrackPanel()->GetTrackCount());
}

Track *TrackControls::FindTrack()
{
   return GetTrack();
}

enum
{
   OnSetNameID = 2000,
   OnMoveUpID,
   OnMoveDownID,
   OnMoveTopID,
   OnMoveBottomID,

   OnCustomizeID,
};

class TrackMenuTable : public PopupMenuTable
{
   TrackMenuTable() : mpData(NULL) {}
   DECLARE_POPUP_MENU(TrackMenuTable);

public:
   static TrackMenuTable &Instance();

private:
   void OnSetName(wxCommandEvent &);
   void OnMoveTrack(wxCommandEvent &event);

   void OnCustomize(wxCommandEvent &);

   void InitMenu(Menu *pMenu, void *pUserData) override;

   void DestroyMenu() override
   {
      mpData = nullptr;
   }

   TrackControls::InitMenuData *mpData;
};

TrackMenuTable &TrackMenuTable::Instance()
{
   static TrackMenuTable instance;
   return instance;
}

void TrackMenuTable::InitMenu(Menu *pMenu, void *pUserData)
{
   mpData = static_cast<TrackControls::InitMenuData*>(pUserData);
   Track *const pTrack = mpData->pTrack;

   TrackList *const tracks = GetActiveProject()->GetTracks();

   pMenu->Enable(OnMoveUpID, tracks->CanMoveUp(pTrack));
   pMenu->Enable(OnMoveDownID, tracks->CanMoveDown(pTrack));
   pMenu->Enable(OnMoveTopID, tracks->CanMoveUp(pTrack));
   pMenu->Enable(OnMoveBottomID, tracks->CanMoveDown(pTrack));
}

BEGIN_POPUP_MENU(TrackMenuTable)
   POPUP_MENU_ITEM(OnSetNameID, _("&Name..."), OnSetName)
   POPUP_MENU_ITEM(OnCustomizeID, _("Customize..."), OnCustomize)
   POPUP_MENU_SEPARATOR()
   POPUP_MENU_ITEM(
      // It is not correct to use KeyStringDisplay here -- wxWidgets will apply
      // its equivalent to the key names passed to menu functions.
      OnMoveUpID,
      _("Move Track &Up") + wxT("\t") +
         (GetActiveProject()->GetCommandManager()->
          GetKeyFromName(wxT("TrackMoveUp"))),
      OnMoveTrack)
   POPUP_MENU_ITEM(
      OnMoveDownID,
      _("Move Track &Down") + wxT("\t") +
         (GetActiveProject()->GetCommandManager()->
          GetKeyFromName(wxT("TrackMoveDown"))),
      OnMoveTrack)
   POPUP_MENU_ITEM(
      OnMoveTopID,
      _("Move Track to &Top") + wxT("\t") +
         (GetActiveProject()->GetCommandManager()->
          GetKeyFromName(wxT("TrackMoveTop"))),
      OnMoveTrack)
   POPUP_MENU_ITEM(
      OnMoveBottomID,
      _("Move Track to &Bottom") + wxT("\t") +
         (GetActiveProject()->GetCommandManager()->
          GetKeyFromName(wxT("TrackMoveBottom"))),
      OnMoveTrack)
END_POPUP_MENU()

void TrackMenuTable::OnSetName(wxCommandEvent &)
{
   Track *const pTrack = mpData->pTrack;
   if (pTrack)
   {
      AudacityProject *const proj = ::GetActiveProject();
      const wxString oldName = pTrack->GetName();
      const wxString newName =
         wxGetTextFromUser(_("Change track name to:"),
         _("Track Name"), oldName);
      if (newName != wxT("")) // wxGetTextFromUser returns empty string on Cancel.
      {
         pTrack->SetName(newName);
         // if we have a linked channel this name should change as well
         // (otherwise sort by name and time will crash).
         if (pTrack->GetLinked())
            pTrack->GetLink()->SetName(newName);

         MixerBoard *const pMixerBoard = proj->GetMixerBoard();
         auto pt = dynamic_cast<PlayableTrack*>(pTrack);
         if (pt && pMixerBoard)
            pMixerBoard->UpdateName(pt);

         proj->PushState(wxString::Format(_("Renamed '%s' to '%s'"),
            oldName.c_str(),
            newName.c_str()),
            _("Name Change"));

         mpData->result = RefreshCode::RefreshAll;
      }
   }
}

void TrackMenuTable::OnMoveTrack(wxCommandEvent &event)
{
   AudacityProject *const project = GetActiveProject();
   AudacityProject::MoveChoice choice;
   switch (event.GetId()) {
   default:
      wxASSERT(false);
   case OnMoveUpID:
      choice = AudacityProject::OnMoveUpID; break;
   case OnMoveDownID:
      choice = AudacityProject::OnMoveDownID; break;
   case OnMoveTopID:
      choice = AudacityProject::OnMoveTopID; break;
   case OnMoveBottomID:
      choice = AudacityProject::OnMoveBottomID; break;
   }

   project->MoveTrack(mpData->pTrack, choice);

   // MoveTrack already refreshed TrackPanel, which means repaint will happen.
   // This is a harmless redundancy:
   mpData->result = RefreshCode::RefreshAll;
}

TrackInfo::TCPLines::iterator findTCPLine( TrackInfo::TCPLines &lines, int yy )
{
   auto it = lines.begin(), end = lines.end();
   while (it != end && yy >= it->height )
      yy -= it++ ->height;
   if ( yy >= 0 )
      return it;
   return end;
}

class CustomizePanel : public wxPanelWrapper
{
public:
   CustomizePanel
      ( wxWindow *parent, const Track &track,
        std::vector<TrackInfo::TCPLine> &topLines,
        const std::vector<TrackInfo::TCPLine> &bottomLines )
      : wxPanelWrapper{
         parent, wxID_ANY, wxDefaultPosition,
         wxSize{
            kTrackInfoWidth - kLeftMargin,
            TrackInfo::TotalTCPLines( topLines, true ) +
            TrackInfo::TotalTCPLines( bottomLines, false ) + 1
         }
      }
      , mTrack{ track }
      , mTopLines{ topLines }
      , mBottomLines{ bottomLines }
      , mpLine{ mTopLines.end() }
   {}

   void OnPaint(wxPaintEvent &);
   void OnMouseEvents(wxMouseEvent &event);

   const Track &mTrack;
   std::vector<TrackInfo::TCPLine> &mTopLines;
   const std::vector<TrackInfo::TCPLine> &mBottomLines;

   std::vector<TrackInfo::TCPLine>::iterator mpLine;

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CustomizePanel, wxPanelWrapper)
   EVT_PAINT(CustomizePanel::OnPaint)
   EVT_MOUSE_EVENTS(CustomizePanel::OnMouseEvents)
END_EVENT_TABLE()

void CustomizePanel::OnPaint(wxPaintEvent & WXUNUSED(evt))
{
   wxPaintDC dc{ this };
   auto rect = GetClientRect();

   // Paint background as if a selected TCP
   AColor::MediumTrackInfo(&dc, true);
   dc.DrawRectangle(rect);

   TrackInfo::DrawItems
      ( &dc, rect,
        nullptr, mTopLines, mBottomLines, TrackPanel::IsUncaptured, false );
}

void CustomizePanel::OnMouseEvents(wxMouseEvent &event)
{
   if (event.LeftDown()) {
      mpLine = findTCPLine( mTopLines, event.m_y );
   }
   else if (event.Dragging() && mpLine != mTopLines.end()) {
      auto pNewLine = findTCPLine( mTopLines, event.m_y );
      if ( pNewLine != mTopLines.end() ) {
         if (pNewLine < mpLine)
            std::swap( *mpLine, *(mpLine - 1) ), --mpLine, Refresh();
         else if (pNewLine > mpLine)
            std::swap( *mpLine, *(mpLine + 1) ), ++mpLine, Refresh();
      }
   }
   else if (event.ButtonUp()) {
      mpLine = mTopLines.end();
   }
}

void TrackMenuTable::OnCustomize(wxCommandEvent &)
{
   wxDialogWrapper dlg(mpData->pParent, wxID_ANY, wxString(_("Customize Controls")));
   dlg.SetName(dlg.GetTitle());

   auto &lines = TrackInfo::GetTCPLines( *mpData->pTrack );
   CustomizePanel panel{
      &dlg, *mpData->pTrack, lines, TrackInfo::CommonTrackTCPBottomLines };

   ShuttleGui S(&dlg, eIsCreating);
   S.StartVerticalLay(true);
   {
      S.AddVariableText(_("Drag up or down to rearrange items"));
      S.AddWindow( &panel );
   }
   S.EndVerticalLay();
   S.AddStandardButtons();

   dlg.Layout();
   dlg.Fit();
   dlg.CenterOnParent();
   if (dlg.ShowModal() == wxID_CANCEL)
      return;

   // to do: make changes persistent in preferences too
   lines = panel.mTopLines;

   mpData->result = RefreshCode::RefreshAll;
}

unsigned TrackControls::DoContextMenu
   (const wxRect &rect, wxWindow *pParent, wxPoint *)
{
   wxRect buttonRect;
   TrackInfo::GetTitleBarRect(*FindTrack(), rect, buttonRect);

   InitMenuData data{ mpTrack, pParent, RefreshCode::RefreshNone };

   const auto pTable = &TrackMenuTable::Instance();
   auto pMenu = PopupMenuTable::BuildMenu(pParent, pTable, &data);

   PopupMenuTable *const pExtension = GetMenuExtension(mpTrack);
   if (pExtension)
      pMenu->Extend(pExtension);

   pParent->PopupMenu
      (pMenu.get(), buttonRect.x + 1, buttonRect.y + buttonRect.height + 1);

   return data.result;
}
