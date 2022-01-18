/**********************************************************************

  Audacity: A Digital Audio Editor

  EffectUI.h

  Leland Lucius

  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2 or later.  See License.txt.

**********************************************************************/

#ifndef __AUDACITY_EFFECTUI_H__
#define __AUDACITY_EFFECTUI_H__

#include "Identifier.h"
#include "EffectHostInterface.h"
#include "Observer.h"
#include "PluginInterface.h"

struct AudioIOEvent;

#include "EffectInterface.h"
#include "widgets/wxPanelWrapper.h" // to inherit
#include "widgets/ThemedDialog.h"

#include "SelectedRegion.h"

#include <wx/bitmap.h>

class AudacityProject;
class Effect;

class wxCheckBox;
class ShuttleGui;

//
class EffectUIHost final : public ThemedDialog
{
public:
   // constructors and destructors
   EffectUIHost(wxWindow *parent,
                AudacityProject &project,
                Effect &effect,
                EffectUIClientInterface &client);
   virtual ~EffectUIHost();

   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

   int ShowModal() override;

   bool Populate(ShuttleGui &S) override;

private:
   wxPanel *BuildButtonBar( wxWindow *parent );

   void OnInitDialog(wxInitDialogEvent & evt);
   void OnErase(wxEraseEvent & evt);
   void OnPaint(wxPaintEvent & evt);
   void OnClose(wxCloseEvent & evt);
   void OnApply(wxCommandEvent & evt);
   void DoCancel();
   void OnCancel(wxCommandEvent & evt);
   void OnHelp(wxCommandEvent & evt);
   void OnDebug(wxCommandEvent & evt);
   void OnMenu(wxCommandEvent & evt);
   void OnPlay(wxCommandEvent & evt);
   void OnRewind(wxCommandEvent & evt);
   void OnFFwd(wxCommandEvent & evt);
   void OnPlayback(AudioIOEvent);
   void OnCapture(AudioIOEvent);
   void OnUserPreset(wxCommandEvent & evt);
   void OnFactoryPreset(wxCommandEvent & evt);
   void OnDeletePreset(wxCommandEvent & evt);
   void OnSaveAs(wxCommandEvent & evt);
   void OnImport(wxCommandEvent & evt);
   void OnExport(wxCommandEvent & evt);
   void OnOptions(wxCommandEvent & evt);
   void OnDefaults(wxCommandEvent & evt);

   void UpdateControls();
   wxBitmap CreateBitmap(const char * const xpm[], bool up, bool pusher);
   void LoadUserPresets();

   void InitializeRealtime();
   void CleanupRealtime();

private:
   Observer::Subscription mSubscription;

   AudacityProject *mProject;
   wxWindow *mParent;
   Effect &mEffect;
   EffectUIClientInterface &mClient;

   RegistryPaths mUserPresets;
   bool mInitialized{ false };
   bool mSupportsRealtime{ false };
   bool mIsGUI;
   bool mIsBatch;

   wxButton *mApplyBtn;
   wxButton *mCloseBtn;
   wxButton *mMenuBtn;
   wxButton *mPlayBtn;
   wxButton *mRewindBtn;
   wxButton *mFFwdBtn;

   wxButton *mPlayToggleBtn;

   wxBitmap mPlayBM;
   wxBitmap mPlayDisabledBM;
   wxBitmap mStopBM;
   wxBitmap mStopDisabledBM;

   bool mDisableTransport;
   bool mPlaying;
   bool mCapturing;

   SelectedRegion mRegion;
   double mPlayPos{ 0.0 };

   bool mDismissed{};

#if wxDEBUG_LEVEL
   // Used only in an assertion
   bool mClosed{ false };
#endif

   DECLARE_EVENT_TABLE()
};

class CommandContext;

namespace  EffectUI {

   AUDACITY_DLL_API
   wxDialog *DialogFactory( wxWindow &parent, EffectHostInterface &host,
      EffectUIClientInterface &client);

   /** Run an effect given the plugin ID */
   // Returns true on success.  Will only operate on tracks that
   // have the "selected" flag set to true, which is consistent with
   // Audacity's standard UI.
   AUDACITY_DLL_API bool DoEffect(
      const PluginID & ID, const CommandContext &context, unsigned flags );

}

class ShuttleGui;

// Obsolescent dialog still used only in Noise Reduction/Removal
class AUDACITY_DLL_API EffectDialog /* not final */ : public wxDialogWrapper
{
public:
   // constructors and destructors
   EffectDialog(wxWindow * parent,
                const TranslatableString & title,
                int type = 0,
                int flags = wxDEFAULT_DIALOG_STYLE,
                int additionalButtons = 0);

   void Init();

   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;
   bool Validate() override;

   // NEW virtuals:
   virtual void PopulateOrExchange(ShuttleGui & S);
   virtual void OnPreview(wxCommandEvent & evt);
   virtual void OnOk(wxCommandEvent & evt);

private:
   int mType;
   int mAdditionalButtons;

   DECLARE_EVENT_TABLE()
   wxDECLARE_NO_COPY_CLASS(EffectDialog);
};

#endif // __AUDACITY_EFFECTUI_H__
