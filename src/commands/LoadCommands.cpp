/**********************************************************************

  Audacity: A Digital Audio Editor

  LoadCommands.cpp

  Dominic Mazzoni
  James Crook

**************************************************************************//**
\class BuiltinCommandsModule
\brief Internal module to auto register all built in commands.  It is closely
modelled on BuiltinEffectsModule
*****************************************************************************/


#include "LoadCommands.h"
#include "AudacityCommand.h"
#include "ModuleManager.h"

#include "CommandManager.h"
#include "../Prefs.h"

namespace {
bool sInitialized = false;
const auto PathStart = "Commands";
static Registry::GroupItem &sRegistry()
{
   static Registry::TransparentGroupItem<> registry{ PathStart };
   return registry;
}
}

struct BuiltinCommandsModule::Entry : Registry::SingleItem {
   TranslatableString visibleName;
   Factory factory;
   Entry( const ComponentInterfaceSymbol &name, Factory factory_)
      : SingleItem( name.Internal() )
      , visibleName{ name.Msgid() }, factory( factory_ ) {}
};

void BuiltinCommandsModule::DoRegistration(
   const ComponentInterfaceSymbol &name, const Factory &factory )
{
   wxASSERT( !sInitialized );
   Registry::RegisterItem( sRegistry(), { "" },
      std::make_unique< Entry >( name, factory ) );
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
   return safenew BuiltinCommandsModule();
}

// ============================================================================
// Register this as a builtin module
// ============================================================================
DECLARE_BUILTIN_MODULE(BuiltinsCommandBuiltin);

///////////////////////////////////////////////////////////////////////////////
//
// BuiltinCommandsModule
//
///////////////////////////////////////////////////////////////////////////////

BuiltinCommandsModule::BuiltinCommandsModule()
{
}

BuiltinCommandsModule::~BuiltinCommandsModule()
{
}

// ============================================================================
// ComponentInterface implementation
// ============================================================================

PluginPath BuiltinCommandsModule::GetPath()
{
   return {};
}

ComponentInterfaceSymbol BuiltinCommandsModule::GetSymbol()
{
   return XO("Builtin Commands");
}

VendorSymbol BuiltinCommandsModule::GetVendor()
{
   return XO("The Audacity Team");
}

wxString BuiltinCommandsModule::GetVersion()
{
   // This "may" be different if this were to be maintained as a separate DLL
   return AUDACITY_VERSION_STRING;
}

TranslatableString BuiltinCommandsModule::GetDescription()
{
   return XO("Provides builtin commands to Audacity");
}

// ============================================================================
// ModuleInterface implementation
// ============================================================================

bool BuiltinCommandsModule::Initialize()
{
   struct Visitor : Registry::Visitor{
      CommandHash *pCommands;
      void Visit( Registry::SingleItem& item, const Path& ) override
      {
         auto pEntry = static_cast< Entry* >( &item );
         auto path = wxString(BUILTIN_GENERIC_COMMAND_PREFIX) + pEntry->name.GET();
         (*pCommands)[ path ] = pEntry;
      }
   } visitor;
   visitor.pCommands = &mCommands;
   Registry::TransparentGroupItem<> top{ PathStart };
   Registry::Visit( visitor, &top, &sRegistry() );
   sInitialized = true;
   return true;
}

void BuiltinCommandsModule::Terminate()
{
   // Nothing to do here
   return;
}

EffectFamilySymbol BuiltinCommandsModule::GetOptionalFamilySymbol()
{
   // Commands are not enabled and disabled in EffectsPrefs
   return {};
}

const FileExtensions &BuiltinCommandsModule::GetFileExtensions()
{
   static FileExtensions empty;
   return empty;
}

bool BuiltinCommandsModule::AutoRegisterPlugins(PluginManagerInterface & pm)
{
   TranslatableString ignoredErrMsg;
   for (const auto &pair : mCommands)
   {
      const auto &path = pair.first;
      if (!pm.IsPluginRegistered(path, &pair.second->visibleName))
      {
         // No checking of error ?
         // Uses Generic Registration, not Default.
         // Registers as TypeGeneric, not TypeEffect.
         DiscoverPluginsAtPath(path, ignoredErrMsg, &pm);
      }
   }

   // We still want to be called during the normal registration process
   return false;
}

PluginPaths BuiltinCommandsModule::FindPluginPaths(PluginManagerInterface & WXUNUSED(pm))
{
   PluginPaths names;
   for ( const auto &pair : mCommands )
      names.push_back( pair.first );
   return names;
}

unsigned BuiltinCommandsModule::DiscoverPluginsAtPath(
   const PluginPath & path, TranslatableString &errMsg,
   PluginManagerInterface *pPluginManager )
{
   errMsg = {};
   auto Command = Instantiate(path);
   if (Command)
   {
      if ( pPluginManager )
         pPluginManager->RegisterPlugin( this, Command.get() );
      return 1;
   }

   errMsg = XO("Unknown built-in command name");
   return 0;
}

bool BuiltinCommandsModule::IsPluginValid(const PluginPath & path, bool bFast)
{
   // bFast is unused as checking in the list is fast.
   static_cast<void>(bFast); // avoid unused variable warning
   return mCommands.find( path ) != mCommands.end();
}

std::shared_ptr< ComponentInterface > BuiltinCommandsModule::CreateInstance(
   const PluginPath & path )
{
   // Acquires a resource for the application.
   return Instantiate(path);
}

// ============================================================================
// BuiltinCommandsModule implementation
// ============================================================================

std::unique_ptr<AudacityCommand> BuiltinCommandsModule::Instantiate(const PluginPath & path)
{
   wxASSERT(path.StartsWith(BUILTIN_GENERIC_COMMAND_PREFIX));
   auto iter = mCommands.find( path );
   if ( iter != mCommands.end() )
      return iter->second->factory();

   wxASSERT( false );
   return nullptr;
}
