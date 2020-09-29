/**********************************************************************

  Audacity: A Digital Audio Editor

  @file CommandDispatch.cpp
  @brief implements function HandleTextualCommand

  Paul Licameli split from MacroCommands.cpp

**********************************************************************/

#include "CommandDispatch.h"

#include "CommandManager.h"
#include "../PluginManager.h"
#include "../effects/EffectManager.h"
#include "../effects/EffectUI.h"

bool HandleTextualCommand( CommandManager &commandManager,
   const CommandID & Str,
   const CommandContext & context, CommandFlag flags, bool alwaysEnabled)
{
   switch ( commandManager.HandleTextualCommand(
      Str, context, flags, alwaysEnabled) ) {
   case CommandManager::CommandSuccess:
      return true;
   case CommandManager::CommandFailure:
      return false;
   case CommandManager::CommandNotFound:
   default:
      break;
   }

   // Not one of the singleton commands.
   // We could/should try all the list-style commands.
   // instead we only try the effects.
   PluginManager & pm = PluginManager::Get();
   EffectManager & em = EffectManager::Get();
   const PluginDescriptor *plug = pm.GetFirstPlugin(PluginTypeEffect);
   while (plug)
   {
      if (em.GetCommandIdentifier(plug->GetID()) == Str)
      {
         return EffectUI::DoEffect(
            plug->GetID(), context,
            EffectManager::kConfigured);
      }
      plug = pm.GetNextPlugin(PluginTypeEffect);
   }

   return false;
}

