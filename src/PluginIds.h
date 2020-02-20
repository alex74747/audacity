/**********************************************************************

  Audacity: A Digital Audio Editor

  PluginIds.h

  Paul Licameli split from PluginManager.h

**********************************************************************/

#ifndef __AUDACITY_PLUGIN_IDS__
#define __AUDACITY_PLUGIN_IDS__

#include "audacity/Types.h"

class ModuleInterface;
class CommandDefinitionInterface;
class EffectDefinitionInterface;
class ImporterInterface;

enum PluginType : unsigned char
{
   PluginTypeNone = 0,          // 2.1.0 placeholder entries...not used by 2.1.1 or greater
   PluginTypeStub =1,               // Used for plugins that have not yet been registered
   PluginTypeEffect =1<<1,
   PluginTypeAudacityCommand=1<<2,
   PluginTypeExporter=1<<3,
   PluginTypeImporter=1<<4,
   PluginTypeModule=1<<5,
};

namespace PluginIds
{
   // This string persists in configuration files
   // So config compatibility will break if it is changed across Audacity versions
   wxString GetPluginTypeString(PluginType type);

   PluginID GetProviderID(ModuleInterface *module);
   PluginID GetCommandID(CommandDefinitionInterface *command);
   PluginID GetEffectID(EffectDefinitionInterface *effect);
   PluginID GetImporterID(ImporterInterface *importer);
};

#endif
