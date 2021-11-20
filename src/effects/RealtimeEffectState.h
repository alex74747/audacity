/**********************************************************************

 Audacity: A Digital Audio Editor

 @file RealtimeEffectState.h

 Paul Licameli split from RealtimeEffectManager.cpp

 *********************************************************************/

#ifndef __AUDACITY_REALTIMEEFFECTSTATE_H__
#define __AUDACITY_REALTIMEEFFECTSTATE_H__

#include <atomic>
#include <unordered_map>
#include <mutex>
#include <vector>

#include <wx/defs.h>

#include "audacity/Types.h"
#include "XMLTagHandler.h"

class EffectProcessor;
class Effect;
class Track;

class wxWindow;

class RealtimeEffectState : public XMLTagHandler
{  
public:
   explicit RealtimeEffectState();
   explicit RealtimeEffectState(const PluginID & id);
   ~RealtimeEffectState();

   void SetID(const PluginID & id);
   EffectProcessor *GetEffect();

   bool IsPreFade();
   void PreFade(bool pre);

   bool IsActive() const;

   bool IsBypassed();
   void Bypass(bool Bypass);

   bool Initialize(double rate);
   bool AddProcessor(Track *track, unsigned chans, float rate);
   bool ProcessStart();
   size_t Process(Track *track, unsigned chans, float **inbuf,  float **outbuf, size_t numSamples);
   bool ProcessEnd();
   bool Finalize();

   void CloseEditor();

   bool HandleXMLTag(
      const std::string_view &tag, const AttributesList &attrs) override;
   void HandleXMLEndTag(const std::string_view &tag) override;
   XMLTagHandler *HandleXMLChild(const std::string_view &tag) override;
   void WriteXML(XMLWriter &xmlFile);

private:
   PluginID mID;
   std::unique_ptr<EffectProcessor> mEffect;
   bool mInitialized;

   std::unordered_map<Track *, int> mGroups;

   bool mBypass;

   bool mPre;

   std::mutex mMutex;

   wxString mParameters;
};

#endif // __AUDACITY_REALTIMEEFFECTSTATE_H__
