/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   File License: wxWidgets

   Dan Horgan

******************************************************************//**

\file Command.h
\brief Contains declaration of Command base class.


*//*******************************************************************/

#ifndef __COMMAND__
#define __COMMAND__

#include "CommandSignature.h"
#include "../commands/AudacityCommand.h"

class AudacityApp;
class CommandContext;
class CommandOutputTargets;

// Abstract base class for command interface.  
class OldStyleCommand /* not final */
{
public:
   AudacityProject &mProject;

   OldStyleCommand(AudacityProject &project) : mProject{ project } {};
   virtual ~OldStyleCommand() { }
   virtual ComponentInterfaceSymbol GetSymbol() = 0;
   virtual CommandSignature &GetSignature() = 0;
   virtual bool SetParameter(const wxString &paramName, const wxVariant &paramValue);
   virtual bool Apply(const CommandContext &context) = 0;
};

using OldStyleCommandPointer = std::shared_ptr<OldStyleCommand>;

// Decorator command that performs the given command and then outputs a status
// message according to the result
class ApplyAndSendResponse final
{
   OldStyleCommandPointer mCommand;
public:
   ApplyAndSendResponse(
      const OldStyleCommandPointer &cmd, std::unique_ptr<CommandOutputTargets> &target);
   bool DoApply();
   ComponentInterfaceSymbol GetSymbol();
   bool SetParameter(const wxString &paramName, const wxVariant &paramValue);
   std::unique_ptr<const CommandContext> mCtx;

};

class AUDACITY_DLL_API CommandImplementation /* not final */
   : public OldStyleCommand
{
private:
   OldStyleCommandType &mType;
   ParamValueMap mParams;
   ParamBoolMap mSetParams;

   /// Using the command signature, looks up a possible parameter value and
   /// checks whether it passes the validator.
   bool Valid(const wxString &paramName, const wxVariant &paramValue);

protected:
   // Convenience methods for allowing subclasses to access parameters
   void TypeCheck(const wxString &typeName,
                  const wxString &paramName,
                  const wxVariant &param);
   void CheckParam(const wxString &paramName);
   bool HasParam( const wxString &paramName);
   bool GetBool(const wxString &paramName);
   long GetLong(const wxString &paramName);
   double GetDouble(const wxString &paramName);
   wxString GetString(const wxString &paramName);

public:
   /// Constructor should not be called directly; only by a factory which
   /// ensures name and params are set appropriately for the command.
   CommandImplementation(AudacityProject &project, OldStyleCommandType &type);

   virtual ~CommandImplementation();

   /// An instance method for getting the command name (for consistency)
   ComponentInterfaceSymbol GetSymbol() override;

   /// Get the signature of the command
   CommandSignature &GetSignature() override;

   /// Attempt to one of the command's parameters to a particular value.
   /// (Note: wxVariant is reference counted)
   bool SetParameter(const wxString &paramName, const wxVariant &paramValue) override;
};

#endif /* End of include guard: __COMMAND__ */
