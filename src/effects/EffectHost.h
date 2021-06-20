/*!*********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file EffectHost.h
 
 Paul Licameli split from Effect.h
 
 **********************************************************************/

#ifndef __AUDACITY_EFFECT_HOST__
#define __AUDACITY_EFFECT_HOST__

#include "Effect.h"

class EffectHost final : public Effect
{
public:
   explicit EffectHost(EffectUIClientInterface &client);
   ~EffectHost() override;

   bool Startup() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   EffectFamilySymbol GetFamily() override;
   bool IsInteractive() override;
   bool IsDefault() override;
   bool SupportsRealtime() override;
   bool SupportsAutomation() override;

   bool GetAutomationParameters(CommandParameters & parms) override;
   bool SetAutomationParameters(CommandParameters & parms) override;

   bool LoadUserPreset(const RegistryPath & name) override;
   bool SaveUserPreset(const RegistryPath & name) override;

   RegistryPaths GetFactoryPresets() override;
   bool LoadFactoryPreset(int id) override;
   bool LoadFactoryDefaults() override;

   // ComponentInterface implementation

   PluginPath GetPath() override;

   ComponentInterfaceSymbol GetSymbol() override;

   VendorSymbol GetVendor() override;
   wxString GetVersion() override;
   TranslatableString GetDescription() override;

   // EffectUIHostInterface implementation

   EffectDefinitionInterface &GetDefinition() override;
   EffectProcessor &GetProcessor() override;
   EffectUIClientInterface &GetClient() override;

   // EffectUIClientInterface implementation

   bool SetHost(EffectHostInterface *host) override;
   
   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;

   int GetMidiInCount() override;
   int GetMidiOutCount() override;

   sampleCount GetLatency() override;
   size_t GetTailSize() override;

   void SetSampleRate(double rate) override;
   size_t SetBlockSize(size_t maxBlockSize) override;
   size_t GetBlockSize() const override;

   bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) override;
   bool ProcessFinalize() override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;

   bool RealtimeInitialize() override;
   bool RealtimeAddProcessor(unsigned numChannels, float sampleRate) override;
   bool RealtimeFinalize() override;
   bool RealtimeSuspend() override;
   bool RealtimeResume() override;
   bool RealtimeProcessStart() override;
   size_t RealtimeProcess(int group,
                                       float **inbuf,
                                       float **outbuf,
                                       size_t numSamples) override;
   bool RealtimeProcessEnd() override;

private:
   // For client driver
   EffectUIClientInterface &mClient;
};

#endif
