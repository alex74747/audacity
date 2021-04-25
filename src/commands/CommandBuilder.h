/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   File License: wxWidgets

   Dan Horgan

******************************************************************//**

\file CommandBuilder.h
\brief Contains declaration of CommandBuilder class.

*//*******************************************************************/

#ifndef __COMMANDBUILDER__
#define __COMMANDBUILDER__

#include "../MemoryX.h"

class ApplyAndSendResponse;
class AudacityProject;
class ResponseTarget;
using ResponseTargetPointer = std::shared_ptr<ResponseTarget>;
class OldStyleCommand;
using OldStyleCommandPointer = std::shared_ptr<OldStyleCommand>;
class wxString;

// CommandBuilder has the task of validating and interpreting a command string.
// If the string represents a valid command, it builds the command object.

class CommandBuilder
{
      using ResponderPtr = std::shared_ptr<ApplyAndSendResponse>;
   private:
      bool mValid;
      ResponseTargetPointer mResponse;
      OldStyleCommandPointer mCommand;
      ResponderPtr mResponder;
      wxString mError;

      void Failure(const wxString &msg = {});
      void Success(const ResponderPtr &responder);
      void BuildCommand( AudacityProject *project,
         const wxString &cmdName, const wxString &cmdParams);
      void BuildCommand( AudacityProject *project, const wxString &cmdString);
   public:
      CommandBuilder(AudacityProject *project, const wxString &cmdString);
      CommandBuilder(AudacityProject *project, const wxString &cmdName,
                     const wxString &cmdParams);
      ~CommandBuilder();
      bool WasValid();
      OldStyleCommandPointer GetCommand();
      wxString GetResponse();
};
#endif /* End of include guard: __COMMANDBUILDER__ */
