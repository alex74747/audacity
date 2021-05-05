/**********************************************************************

 Audacity: A Digital Audio Editor

 RealtimeEffectList.h

 *********************************************************************/

#ifndef __AUDACITY_REALTIMEEFFECTLIST_H__
#define __AUDACITY_REALTIMEEFFECTLIST_H__

#include <vector>

#include "TrackAttachment.h"

class AudacityProject;

class RealtimeEffectState;

class Track;

class RealtimeEffectList final
   : public TrackAttachment
   , public std::enable_shared_from_this<RealtimeEffectList>
{
   RealtimeEffectList(const RealtimeEffectList &) = delete;
   RealtimeEffectList &operator=(const RealtimeEffectList &) = delete;

public:
   RealtimeEffectList();
   virtual ~RealtimeEffectList();

   static RealtimeEffectList &Get(AudacityProject &project);
   static const RealtimeEffectList &Get(const AudacityProject &project);

   static RealtimeEffectList &Get(Track &track);
   static const RealtimeEffectList &Get(const Track &track);

   void Visit(
      std::function<void(RealtimeEffectState &state, bool bypassed)> func);

   using States = std::vector<std::unique_ptr<RealtimeEffectState>>;
   States & GetStates();
   RealtimeEffectState &GetState(size_t index);

private:
   States mStates;
};

#endif // __AUDACITY_REALTIMEEFFECTLIST_H__
