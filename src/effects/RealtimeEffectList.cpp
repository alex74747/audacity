/**********************************************************************
 
  Audacity: A Digital Audio Editor
 
  RealtimeEffectList.cpp
 
 *********************************************************************/

#include "RealtimeEffectList.h"
#include "RealtimeEffectState.h"

#include "RealtimeEffectUI.h"

#include "EffectInterface.h"

#include "Project.h"
#include "Track.h"

#include <wx/time.h>

RealtimeEffectList::RealtimeEffectList(bool deleteUI)
{
   mUI = nullptr;
   mDeleteUI = deleteUI;
   mBypass = false;
   mSuspend = 0;
}

RealtimeEffectList::~RealtimeEffectList()
{
   if (mDeleteUI && mUI)
   {
      delete mUI;
   }
}

static const AttachedProjectObjects::RegisteredFactory masterEffects
{
   [](AudacityProject &project)
   {
      return std::make_shared<RealtimeEffectList>(false);
   }
};

RealtimeEffectList &RealtimeEffectList::Get(AudacityProject &project)
{
   return project.AttachedObjects::Get<RealtimeEffectList>(masterEffects);
}

const RealtimeEffectList &RealtimeEffectList::Get(const AudacityProject &project)
{
   return Get(const_cast<AudacityProject &>(project));
}

static const AttachedTrackObjects::RegisteredFactory trackEffects
{
   [](Track &track)
   {
      return std::make_shared<RealtimeEffectList>(true);
   }
};

RealtimeEffectList &RealtimeEffectList::Get(Track &track)
{
   return track.AttachedObjects::Get<RealtimeEffectList>(trackEffects);
}

const RealtimeEffectList &RealtimeEffectList::Get(const Track &track)
{
   return Get(const_cast<Track &>(track));
}

bool RealtimeEffectList::IsBypassed() const
{
   return mBypass;
}

void RealtimeEffectList::Bypass(bool bypass)
{
   mBypass = bypass;
   if (mUI)
   {
      mUI->Rebuild();
   }
}

void RealtimeEffectList::Suspend()
{
   mSuspend++;
}

void RealtimeEffectList::Resume() noexcept
{
   mSuspend--;
}

void RealtimeEffectList::Visit(
   std::function<void(RealtimeEffectState &state, bool bypassed)> func)
{
   for (auto &state : mStates)
   {
      func(*state, IsBypassed() || !state->IsActive());
   }
}

void RealtimeEffectList::Swap(int index1, int index2)
{
   std::swap(mStates[index1], mStates[index2]);
}

RealtimeEffectList::States & RealtimeEffectList::GetStates()
{
   return mStates;
}

RealtimeEffectState &RealtimeEffectList::GetState(size_t index)
{
   return *mStates[index];
}

void RealtimeEffectList::Show(RealtimeEffectManager *manager,
                              const TranslatableString &title,
                              wxPoint pos /* = wxDefaultPosition */)
{
   bool existed = (mUI != nullptr);
   if (!existed)
   {
      mUI = safenew RealtimeEffectUI(manager, title, *this);
   }

   if (mUI)
   {
      if (!existed)
      {
         mUI->CenterOnParent();
      }

      mUI->Show();
      if (!existed && pos != wxDefaultPosition)
      {
         mUI->Move(pos);
      }
   }
}
