/**********************************************************************

  Audacity: A Digital Audio Editor

  PluginIds.cpp

  Paul Licameli split from PluginManager.cpp

**********************************************************************/

#include "PluginIds.h"
#include "PluginIds.h"

#include "audacity/ModuleInterface.h"

// This string persists in configuration files
// So config compatibility will break if it is changed across Audacity versions
wxString PluginIds::GetPluginTypeString(PluginType type)
{
   wxString str;

   switch (type)
   {
   default:
   case PluginTypeNone:
      str = wxT("Placeholder");
      break;
   case PluginTypeStub:
      str = wxT("Stub");
      break;
   case PluginTypeEffect:
      str = wxT("Effect");
      break;
   case PluginTypeAudacityCommand:
      str = wxT("Generic");
      break;
   case PluginTypeExporter:
      str = wxT("Exporter");
      break;
   case PluginTypeImporter:
      str = wxT("Importer");
      break;
   case PluginTypeModule:
      str = wxT("Module");
      break;
   }

   return str;
}

PluginID PluginIds::GetProviderID(ModuleInterface *module)
{
   return wxString::Format(wxT("%s_%s_%s_%s_%s"),
                           GetPluginTypeString(PluginTypeModule),
                           wxEmptyString,
                           module->GetVendor().Internal(),
                           module->GetSymbol().Internal(),
                           module->GetPath());
}

PluginID PluginIds::GetCommandID(ComponentInterface *command)
{
   return wxString::Format(wxT("%s_%s_%s_%s_%s"),
                           GetPluginTypeString(PluginTypeAudacityCommand),
                           wxEmptyString,
                           command->GetVendor().Internal(),
                           command->GetSymbol().Internal(),
                           command->GetPath());
}

PluginID PluginIds::GetEffectID(EffectDefinitionInterface *effect)
{
   return wxString::Format(wxT("%s_%s_%s_%s_%s"),
                           GetPluginTypeString(PluginTypeEffect),
                           effect->GetFamily().Internal(),
                           effect->GetVendor().Internal(),
                           effect->GetSymbol().Internal(),
                           effect->GetPath());
}

PluginID PluginIds::GetImporterID(ImporterInterface *importer)
{
   return wxString::Format(wxT("%s_%s_%s_%s_%s"),
                           GetPluginTypeString(PluginTypeImporter),
                           wxEmptyString,
                           importer->GetVendor().Internal(),
                           importer->GetSymbol().Internal(),
                           importer->GetPath());
}

