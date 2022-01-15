/**********************************************************************

Audacity: A Digital Audio Editor

LabelTrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/


#include "LabelTrackControls.h"

#include "LabelTrackView.h"
#include "../../../HitTestResult.h"
#include "../../../LabelTrack.h"
#include "../../../widgets/PopupMenuTable.h"
#include "Prefs.h"
#include "../../../RefreshCode.h"
#include "../../../ShuttleGui.h"
#include "../../../widgets/wxPanelWrapper.h"
#include <wx/fontenum.h>

LabelTrackControls::~LabelTrackControls()
{
}

std::vector<UIHandlePtr> LabelTrackControls::HitTest
(const TrackPanelMouseState & state,
 const AudacityProject *pProject)
{
   return CommonTrackControls::HitTest(state, pProject);
}

class LabelTrackMenuTable : public PopupMenuTable
{
   LabelTrackMenuTable()
      : PopupMenuTable{ "LabelTrack" }
   {}
   DECLARE_POPUP_MENU(LabelTrackMenuTable);

public:
   static LabelTrackMenuTable &Instance();

   void InitUserData(void *pUserData) override
   {
      mpData = static_cast<CommonTrackControls::InitMenuData*>(pUserData);
   }

   CommonTrackControls::InitMenuData *mpData{};

   void OnSetFont();
};

LabelTrackMenuTable &LabelTrackMenuTable::Instance()
{
   static LabelTrackMenuTable instance;
   return instance;
}

enum
{
   OnSetFontID = 30000,
};

BEGIN_POPUP_MENU(LabelTrackMenuTable)
   BeginSection( "Basic" );
      AppendItem( "Font", OnSetFontID, XXO("&Font..."), POPUP_MENU_FN( OnSetFont ) );
   EndSection();
END_POPUP_MENU()

void LabelTrackMenuTable::OnSetFont()
{
   // Small helper class to enumerate all fonts in the system
   // We use this because the default implementation of
   // wxFontEnumerator::GetFacenames() has changed between wx2.6 and 2.8
   class FontEnumerator : public wxFontEnumerator, public Identifiers
   {
   public:
      FontEnumerator()
      {
         EnumerateFacenames( wxFONTENCODING_SYSTEM, false );
      }

      bool OnFacename(const wxString& font) override
      {
         this->emplace_back(font);
         return true;
      }
   };

   SettingTransaction transaction;

   // Correct for empty facename, or bad preference file:
   // get the name of a really existing font, to highlight by default
   // in the list box
   LabelTrackView::FaceName.Write(
      LabelTrackView::GetFont( LabelTrackView::FaceName.Read() )
         .GetFaceName() );

   /* i18n-hint: (noun) This is the font for the label track.*/
   wxDialogWrapper dlg(mpData->pParent, wxID_ANY, XO("Label Track Font"));
   dlg.SetName();
   ShuttleGui S(&dlg);

   using namespace DialogDefinition;
   S.StartVerticalLay(true);
   {
      S.StartMultiColumn(2,
                         GroupOptions{ wxEXPAND }
                            .StretchyColumn(1)
                            .StretchyRow(0));
      {
         S
            /* i18n-hint: (noun) The name of the typeface*/
            .AddPrompt(XXO("Face name"));

         S
            .Text(XO("Face name"))
            .Position(  wxALIGN_LEFT | wxEXPAND | wxALL )
            .Target( Choice( LabelTrackView::FaceName,
               Verbatim( []() -> Identifiers {
                  return FontEnumerator{};
               }() )
            ) )
            .AddListBox( {} );

         S
            /* i18n-hint: (noun) The size of the typeface*/
            .AddPrompt(XXO("Face size"));

         S
            .Position( wxALIGN_LEFT | wxALL )
            .Style(wxSP_ARROW_KEYS /* | wxSP_VERTICAL */ )
            .Target( LabelTrackView::FontSize )
            .AddSpinCtrl(XXO("Face size"), 0, 48, 8);
      }
      S.EndMultiColumn();

      S
         .AddStandardButtons();
   }
   S.EndVerticalLay();

   dlg.Layout();
   dlg.Fit();
   dlg.CenterOnParent();
   if (dlg.ShowModal() == wxID_CANCEL)
      return;

   dlg.TransferDataFromWindow();
   transaction.Commit();

   LabelTrackView::ResetFont();

   mpData->result = RefreshCode::RefreshAll;
}

PopupMenuTable *LabelTrackControls::GetMenuExtension(Track *)
{
   return &LabelTrackMenuTable::Instance();
}

using DoGetLabelTrackControls = DoGetControls::Override< LabelTrack >;
DEFINE_ATTACHED_VIRTUAL_OVERRIDE(DoGetLabelTrackControls) {
   return [](LabelTrack &track) {
      return std::make_shared<LabelTrackControls>( track.SharedPointer() );
   };
}

using GetDefaultLabelTrackHeight = GetDefaultTrackHeight::Override< LabelTrack >;
DEFINE_ATTACHED_VIRTUAL_OVERRIDE(GetDefaultLabelTrackHeight) {
   return [](LabelTrack &) {
      // Label tracks are narrow
      // Default is to allow two rows so that NEW users get the
      // idea that labels can 'stack' when they would overlap.
      return 73;
   };
}
