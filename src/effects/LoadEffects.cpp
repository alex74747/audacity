/**********************************************************************

  Audacity: A Digital Audio Editor

  LoadEffects.cpp

  Dominic Mazzoni

**************************************************************************//**
\class BuiltinEffectsModule
\brief Internal module to auto register all built in effects.  
*****************************************************************************/


#include "LoadEffects.h"

#include "../Prefs.h"

#include "Effect.h"
#include "ModuleManager.h"
#include "../commands/CommandManager.h"

namespace {

const auto PathStart = "Effects";

bool sInitialized = false;

static Registry::GroupItem &sRegistry()
{
   static Registry::TransparentGroupItem<> registry{ PathStart };
   return registry;
}

}

struct BuiltinEffectsModule::Entry : Registry::SingleItem {
   TranslatableString visibleName;
   Factory factory;
   bool excluded;
   Entry( const ComponentInterfaceSymbol &name,
      BuiltinEffectsModule::Factory factory_, bool excluded_ )
      : SingleItem( name.Internal() )
      , visibleName{ name.Msgid() }
      , factory( factory_ ), excluded( excluded_ )
   {}
};

void BuiltinEffectsModule::DoRegistration(
   const ComponentInterfaceSymbol &name, const Factory &factory, bool excluded )
{
   wxASSERT( !sInitialized );
   Registry::RegisterItem( sRegistry(), { "" },
      std::make_unique< Entry >( name, factory, excluded ) );
}

// ============================================================================
// Module registration entry point
//
// This is the symbol that Audacity looks for when the module is built as a
// dynamic library.
//
// When the module is builtin to Audacity, we use the same function, but it is
// declared static so as not to clash with other builtin modules.
// ============================================================================
DECLARE_MODULE_ENTRY(AudacityModule)
{
   // Create and register the importer
   // Trust the module manager not to leak this
   return safenew BuiltinEffectsModule();
}

// ============================================================================
// Register this as a builtin module
// ============================================================================
DECLARE_BUILTIN_MODULE(BuiltinsEffectBuiltin);

///////////////////////////////////////////////////////////////////////////////
//
// BuiltinEffectsModule
//
///////////////////////////////////////////////////////////////////////////////

BuiltinEffectsModule::BuiltinEffectsModule()
{
}

BuiltinEffectsModule::~BuiltinEffectsModule()
{
}

// ============================================================================
// ComponentInterface implementation
// ============================================================================

PluginPath BuiltinEffectsModule::GetPath()
{
   return {};
}

ComponentInterfaceSymbol BuiltinEffectsModule::GetSymbol()
{
   return XO("Builtin Effects");
}

VendorSymbol BuiltinEffectsModule::GetVendor()
{
   return XO("The Audacity Team");
}

wxString BuiltinEffectsModule::GetVersion()
{
   // This "may" be different if this were to be maintained as a separate DLL
   return AUDACITY_VERSION_STRING;
}

TranslatableString BuiltinEffectsModule::GetDescription()
{
   return XO("Provides builtin effects to Audacity");
}

// ============================================================================
// ModuleInterface implementation
// ============================================================================

bool BuiltinEffectsModule::Initialize()
{
   struct Visitor : Registry::Visitor{
      EffectHash *pCommands;
      void Visit( Registry::SingleItem& item, const Path& ) override
      {
         auto pEntry = static_cast< Entry* >( &item );
         auto path = wxString(BUILTIN_EFFECT_PREFIX) + pEntry->name.GET();
         (*pCommands)[ path ] = pEntry;
      }
   } visitor;
   visitor.pCommands = &mEffects;
   Registry::TransparentGroupItem<> top{ PathStart };
   Registry::Visit( visitor, &top, &sRegistry() );
   sInitialized = true;
   return true;
}

void BuiltinEffectsModule::Terminate()
{
   // Nothing to do here
   return;
}

EffectFamilySymbol BuiltinEffectsModule::GetOptionalFamilySymbol()
{
   // Returns empty, because there should not be an option in Preferences to
   // disable the built-in effects.
   return {};
}

const FileExtensions &BuiltinEffectsModule::GetFileExtensions()
{
   static FileExtensions empty;
   return empty;
}

bool BuiltinEffectsModule::AutoRegisterPlugins(PluginManagerInterface & pm)
{
   TranslatableString ignoredErrMsg;
   for (const auto &pair : mEffects)
   {
      const auto &path = pair.first;
      if (!pm.IsPluginRegistered(path, &pair.second->visibleName))
      {
         if ( pair.second->excluded )
            continue;
         // No checking of error ?
         DiscoverPluginsAtPath(path, ignoredErrMsg, &pm);
      }
   }

   // We still want to be called during the normal registration process
   return false;
}

PluginPaths BuiltinEffectsModule::FindPluginPaths(PluginManagerInterface & WXUNUSED(pm))
{
   PluginPaths names;
   for ( const auto &pair : mEffects )
      names.push_back( pair.first );
   return names;
}

unsigned BuiltinEffectsModule::DiscoverPluginsAtPath(
   const PluginPath & path, TranslatableString &errMsg,
   PluginManagerInterface *pPluginManager )
{
   errMsg = {};
   auto effect = Instantiate(path);
   if (effect)
   {
      if ( pPluginManager )
         pPluginManager->RegisterPlugin( this, effect.get() );
      return 1;
   }

   errMsg = XO("Unknown built-in effect name");
   return 0;
}

bool BuiltinEffectsModule::IsPluginValid(const PluginPath & path, bool bFast)
{
   // bFast is unused as checking in the list is fast.
   static_cast<void>(bFast);
   return mEffects.find( path ) != mEffects.end();
}

std::shared_ptr< ComponentInterface > BuiltinEffectsModule::CreateInstance(
   const PluginPath & path )
{
   // Acquires a resource for the application.
   return Instantiate(path);
}

// ============================================================================
// BuiltinEffectsModule implementation
// ============================================================================

std::unique_ptr<Effect> BuiltinEffectsModule::Instantiate(const PluginPath & path)
{
   wxASSERT(path.StartsWith(BUILTIN_EFFECT_PREFIX));
   auto iter = mEffects.find( path );
   if ( iter != mEffects.end() )
      return iter->second->factory();
 
   wxASSERT( false );
   return nullptr;
}
