/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   File License: wxWidgets

   Dan Horgan

******************************************************************//**

\file BatchEvalCommand.cpp
\brief Contains definitions for the BatchEvalCommand class

*//*******************************************************************/


#include "BatchEvalCommand.h"

#include "CommandContext.h"
#include "CommandDirectory.h"
#include "Project.h"

static CommandDirectory::RegisterType sRegisterType{
   std::make_unique<BatchEvalCommandType>()
};

ComponentInterfaceSymbol BatchEvalCommandType::BuildName()
{
   return { L"BatchCommand", XO("Batch Command") };
}

void BatchEvalCommandType::BuildSignature(CommandSignature &signature)
{
   auto commandNameValidator = std::make_unique<DefaultValidator>();
   signature.AddParameter(L"CommandName", L"", std::move(commandNameValidator));
   auto paramValidator = std::make_unique<DefaultValidator>();
   signature.AddParameter(L"ParamString", L"", std::move(paramValidator));
   auto macroValidator = std::make_unique<DefaultValidator>();
   signature.AddParameter(L"MacroName", L"", std::move(macroValidator));
}

OldStyleCommandPointer BatchEvalCommandType::Create( AudacityProject &project,
   std::unique_ptr<CommandOutputTargets> && WXUNUSED(target))
{
   return std::make_shared<BatchEvalCommand>(project, *this);
}

bool BatchEvalCommand::Apply(const CommandContext & context)
{
   // Uh oh, I need to build a catalog, expensively
   // Maybe it can be built in one long-lived place and shared among command
   // objects instead?
   // The catalog though may change during a session, as it includes the 
   // names of macro commands - so the long-lived copy will need to 
   // be refreshed after macros are added/deleted.
   MacroCommandsCatalog catalog(&context.project);

   wxString macroName = GetString(L"MacroName");
   if (!macroName.empty())
   {
      MacroCommands batch{ context.project };
      batch.ReadMacro(macroName);
      return batch.ApplyMacro(catalog);
   }

   auto cmdName = GetString(L"CommandName");
   wxString cmdParams = GetString(L"ParamString");
   auto iter = catalog.ByCommandId(cmdName);
   const auto friendly = (iter == catalog.end())
      ? Verbatim( cmdName ) // Expose internal name to user, in default of a better one!
      : iter->name.Msgid();

   // Create a Batch that will have just one command in it...
   MacroCommands Batch{ context.project };
   bool bResult = Batch.ApplyCommandInBatchMode(friendly, cmdName, cmdParams, &context);
   // Relay messages, if any.
   wxString Message = Batch.GetMessage();
   if( !Message.empty() )
      context.Status( Message );
   return bResult;
}

BatchEvalCommand::~BatchEvalCommand()
{ }
