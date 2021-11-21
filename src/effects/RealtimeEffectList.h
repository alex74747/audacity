/**********************************************************************

 Audacity: A Digital Audio Editor

 RealtimeEffectList.h

 *********************************************************************/

#ifndef __AUDACITY_REALTIMEEFFECTLIST_H__
#define __AUDACITY_REALTIMEEFFECTLIST_H__

#include <atomic>
#include <mutex>
#include <vector>

#include <wx/defs.h>
#include <wx/gdicmn.h>

#include "audacity/Types.h" // for PluginID
#include "TrackAttachment.h"
#include "XMLTagHandler.h"

class AudacityProject;
class EffectClientInterface;
class Effect;

class RealtimeEffectManager;
class RealtimeEffectState;
class RealtimeEffectUI;

class Track;

class RealtimeEffectList final
   : public TrackAttachment
   , public XMLTagHandler
{
   RealtimeEffectList(const RealtimeEffectList &) = delete;
   RealtimeEffectList &operator=(const RealtimeEffectList &) = delete;

public:
   RealtimeEffectList(bool deleteUI);
   virtual ~RealtimeEffectList();

   static RealtimeEffectList &Get(AudacityProject &project);
   static const RealtimeEffectList &Get(const AudacityProject &project);

   static RealtimeEffectList &Get(Track &track);
   static const RealtimeEffectList &Get(const Track &track);

   bool IsBypassed() const;
   void Bypass(bool bypass);

   void Suspend();
   void Resume() noexcept;

   void Visit(
      std::function<void(RealtimeEffectState &state, bool bypassed)> func);

   void SetPrefade(RealtimeEffectState &state, bool prefade);
   bool HasPrefaders();
   bool HasPostfaders();

   RealtimeEffectState & AddState(const PluginID &id);
   void RemoveState(RealtimeEffectState &state);
   void Swap(int index1, int index2);

   using States = std::vector<std::unique_ptr<RealtimeEffectState>>;
   States & GetStates();
   RealtimeEffectState &GetState(size_t index);

   void Show(RealtimeEffectManager *manager, const TranslatableString &title, wxPoint pos = wxDefaultPosition);

   bool HandleXMLTag(
      const std::string_view &tag, const AttributesList &attrs) override;
   void HandleXMLEndTag(const std::string_view &tag) override;
   XMLTagHandler *HandleXMLChild(const std::string_view &tag) override;
   void WriteXML(XMLWriter &xmlFile) const;

private:
   RealtimeEffectState *DoAdd(const PluginID &id = {});
private:
   States mStates;
   RealtimeEffectUI *mUI;
   bool mDeleteUI;

   bool mBypass;
   int mSuspend;

   int mPrefaders;
   int mPostfaders;
};

#endif // __AUDACITY_REALTIMEEFFECTLIST_H__
