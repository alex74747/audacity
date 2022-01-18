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

RealtimeEffectList::RealtimeEffectList()
{
   mUI = nullptr;
   mBypass = false;
   mSuspend = 0;
   mPrefaders = 0;
   mPostfaders = 0;
}

RealtimeEffectList::~RealtimeEffectList()
{
}

static const AttachedProjectObjects::RegisteredFactory masterEffects
{
   [](AudacityProject &project)
   {
      return std::make_shared<RealtimeEffectList>();
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
      return std::make_shared<RealtimeEffectList>();
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

void RealtimeEffectList::SetPrefade(RealtimeEffectState &state, bool prefade)
{
   state.PreFade(prefade);

   if (state.IsPreFade())
   {
      mPrefaders++;
      mPostfaders--;
   }
   else
   {
      mPostfaders++;
      mPrefaders--;
   }
}

bool RealtimeEffectList::HasPrefaders()
{
   return mPrefaders;
}

bool RealtimeEffectList::HasPostfaders()
{
   return mPostfaders;
}

RealtimeEffectState & RealtimeEffectList::AddState(const PluginID & id)
{
   return *DoAdd(id);
}

void RealtimeEffectList::RemoveState(RealtimeEffectState &state)
{
   auto end = mStates.end();
   auto found = std::find_if(mStates.begin(), end,
      [&](const std::unique_ptr<RealtimeEffectState> &item)
      {
         return item.get() == &state;
      }
   );

   if (found != end)
   {
      mStates.erase(found);
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

bool RealtimeEffectList::HandleXMLTag(
   const std::string_view &tag, const AttributesList &attrs)
{
   if (tag == "effects")
   {
      mBypass = false;

      for (auto pair : attrs) {
         auto attr = pair.first;
         auto value = pair.second;
         if (attr == "bypass")
         {
            bool bValue;
            if (value.TryGet(bValue))
               Bypass(bValue);
         }
      }

      return true;
   }

   return false;
}

XMLTagHandler *RealtimeEffectList::HandleXMLChild(const std::string_view &tag)
{
   if (tag == "effect")
   {
      return DoAdd();
   }

   return nullptr;
}

void RealtimeEffectList::WriteXML(XMLWriter &xmlFile) const
{
   if (mStates.size() == 0)
   {
      return;
   }

   xmlFile.StartTag(wxT("effects"));
   xmlFile.WriteAttr(wxT("bypass"), mBypass);

   for (const auto & state : mStates)
   {
      state->WriteXML(xmlFile);
   }
   
   xmlFile.EndTag(wxT("effects"));
}


RealtimeEffectState *RealtimeEffectList::DoAdd(const PluginID &id)
{
   mStates.emplace_back(std::make_unique<RealtimeEffectState>(id));
   auto & state = mStates.back();

   if (state->IsPreFade())
   {
      mPrefaders++;
   }
   else
   {
      mPostfaders++;
   }

   return state.get();
}
