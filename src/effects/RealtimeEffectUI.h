/**********************************************************************

 Audacity: A Digital Audio Editor

 RealtimeEffectUI.h

 *********************************************************************/

#ifndef __AUDACITY_REALTIMEEFFECTUI_H__
#define __AUDACITY_REALTIMEEFFECTUI_H__

#include "widgets/ThemedDialog.h" // to inherit
#include "audacity/Types.h" // for PluginID

#include <memory>
#include <vector>
#include <wx/timer.h>
#include <wx/weakref.h>

class AButton;
class Grid;
class wxGridEvent;
class ShuttleGui;
class RealtimeEffectManager;
class RealtimeEffectList;
class RealtimeEffectState;

class RealtimeEffectUI final : public ThemedDialog,
                               public PrefsListener
{
public:
   RealtimeEffectUI(RealtimeEffectManager *manager,
                    const TranslatableString &title,
                    RealtimeEffectList &list);
   virtual ~RealtimeEffectUI();

   static void ShowUI( RealtimeEffectList &list,
      RealtimeEffectManager *manager,
      const TranslatableString &title,
      wxPoint pos = wxDefaultPosition);

   void Rebuild();

   void OnClose(wxCommandEvent & evt) override;

private:
   void Untrack();
   bool Populate(ShuttleGui &S) override;
   void PopulateOrExchange(ShuttleGui &S);
   void Add(RealtimeEffectState &state);

   AButton *CreateButton(wxWindowID id,
                         wxString name,
                         TranslatableString tip,
                         const char *const *up,
                         const char *const *down = nullptr,
                         bool toggle = false,
                         int style = 0,
                         bool add = true);
   void CreateImages(wxImage images[5],
                     const char *const *up,
                     const char *const *down = nullptr);

   int GetEffectIndex(wxWindow *win);
   void MoveRowUp(int row);

   void OnTimer(wxTimerEvent & evt);

   void OnAdd(wxCommandEvent & evt);
   void OnBypass(wxCommandEvent & evt);

   void OnPower(wxCommandEvent & evt);
   void OnEditor(wxCommandEvent & evt);
   void OnPrePost(wxCommandEvent & evt);
   void OnUp(wxCommandEvent & evt);
   void OnDown(wxCommandEvent & evt);
   void OnRemove(wxCommandEvent & evt);

   void OnIdle(wxIdleEvent &evt);

   // PrefsListener
   virtual void UpdatePrefs() override;

private:

   //
   //
   //
   class Selector final : public ThemedDialog
   {
   public:
      Selector(wxWindow *parent, PluginID & id);
      virtual ~Selector();

   private:
      enum
      {
         COL_Effect,
         COL_Type,
         COL_Path,

         COL_COUNT
      };

      struct ItemData
      {
         PluginID id;
         wxString name;
         wxString type;
         PluginPath path;
      };

      using Items = std::vector<ItemData>;

      bool Populate(ShuttleGui &S) override;
      void PopulateOrExchange(ShuttleGui &S);
      void RegenerateEffectsList();
      void Exit(bool returnID);

      void DoSort(int col);

      void OnSort(wxGridEvent &evt);
      void OnKey(wxKeyEvent &evt);
      void OnDClick(wxGridEvent &evt);

      bool IsEscapeKey(const wxKeyEvent &evt) override;

   private:
      PluginID &mID;

      Items mItems;

      int mSortColumn;
      bool mSortDirection;

      Grid *mEffects;

      DECLARE_EVENT_TABLE();
   };

   RealtimeEffectManager *mManager;
   const std::weak_ptr<RealtimeEffectList> mpList;
   TranslatableString mTitle;

   wxPanel *mTitleBar;
   wxStaticText *mTitleText;
   AButton *mClose;

   AButton *mBypass;
   wxStaticText *mLatency;
   std::chrono::microseconds mLastLatency{ -1 };

   std::unique_ptr<Selector> mSelector;

   wxTimer mTimer;

   wxPoint mDragOffset;
   wxPoint mLastPos;
   bool mDragging;

   wxFlexGridSizer *mMainSizer;
   int mIDCounter;

   bool mLastBypassed{ false };

   DECLARE_EVENT_TABLE()
   static std::vector<wxWeakRef<RealtimeEffectUI>> sDialogs;
};

#endif // __AUDACITY_REALTIMEEFFECTUI_H__
