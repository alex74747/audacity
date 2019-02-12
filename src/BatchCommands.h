/**********************************************************************

  Audacity: A Digital Audio Editor

  MacroCommands.h

  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_BATCH_COMMANDS_DIALOG__
#define __AUDACITY_BATCH_COMMANDS_DIALOG__

#include <wx/defs.h>

#include "export/Export.h"
#include "commands/CommandFlag.h"
#include "audacity/ComponentInterface.h" // for ComponentInterfaceSymbol

class Effect;
class CommandContext;
class CommandManager;
class AudacityProject;

// MacroName is a distinct string-wrapper type, which is permissible to be
// shown to the user.  The name is either that of a file the user made, or
// the (localized) name of one of a few built-in macros.
using MacroName = Identifier;
using MacroNames = std::vector<MacroName>;

class MacroCommandsCatalog {
public:
   // A triple of user-visible name, internal string identifier and type/help string.
   struct Entry {
      ComponentInterfaceSymbol name;
      TranslatableString category;
   };
   using Entries = std::vector<Entry>;

   MacroCommandsCatalog( const AudacityProject *project );

   // binary search
   Entries::const_iterator ByFriendlyName( const TranslatableString &friendlyName ) const;
   // linear search
   Entries::const_iterator ByCommandId( const CommandID &commandId ) const;

   // Lookup by position as sorted by friendly name
   const Entry &operator[] ( size_t index ) const { return mCommands[index]; }

   Entries::const_iterator begin() const { return mCommands.begin(); }
   Entries::const_iterator end() const { return mCommands.end(); }

private:
   // Sorted by friendly name
   Entries mCommands;
};

// Stores information for one macro
class MacroCommands final {
 public:
   static bool DoAudacityCommand(
      const PluginID & ID, const CommandContext & context, unsigned flags );

   // constructors and destructors
   MacroCommands( AudacityProject &project );
 public:
   bool ApplyMacro( const MacroCommandsCatalog &catalog,
      const wxString & filename = {});
   static bool HandleTextualCommand( CommandManager &commandManager,
      const CommandID & Str,
      const CommandContext & context, CommandFlag flags, bool alwaysEnabled);
   bool ApplyCommand( const TranslatableString &friendlyCommand,
      const CommandID & command, const wxString & params,
      CommandContext const * pContext=NULL );
   bool ApplyCommandInBatchMode( const TranslatableString &friendlyCommand,
      const CommandID & command, const wxString &params,
      CommandContext const * pContext = NULL);
   bool ApplyEffectCommand(
      const PluginID & ID, const TranslatableString &friendlyCommand,
      const CommandID & command,
      const wxString & params, const CommandContext & Context);
   bool ReportAndSkip( const TranslatableString & friendlyCommand, const wxString & params );
   void AbortBatch();

   // These commands do not depend on the command list.
   static void MigrateLegacyChains();
   static MacroNames GetNames();
   static MacroNames GetNamesOfDefaultMacros();

   static wxString GetCurrentParamsFor(const CommandID & command);
   static wxString PromptForParamsFor(
      const CommandID & command, const wxString & params, wxWindow &parent);
   static wxString PromptForPresetFor(const CommandID & command, const wxString & params, wxWindow *parent);

   // These commands do depend on the command list.
   void ResetMacro();

   void RestoreMacro(const MacroName & name);
   wxString ReadMacro(const MacroName & macro, wxWindow *parent = nullptr);
   wxString WriteMacro(const MacroName & macro, wxWindow *parent = nullptr);
   bool AddMacro(const MacroName & macro);
   bool DeleteMacro(const MacroName & name);
   bool RenameMacro(const MacroName & oldmacro, const MacroName & newmacro);

   void AddToMacro(const CommandID & command, int before = -1);
   void AddToMacro(const CommandID & command, const wxString & params, int before = -1);

   void DeleteFromMacro(int index);
   CommandID GetCommand(int index);
   wxString GetParams(int index);
   int GetCount();
   TranslatableString GetMessage(){ return mMessage;};
   void AddToMessage(const TranslatableString & msgIn ){ mMessage += msgIn;};

   bool IsFixed(const wxString & name);

   void Split(const wxString & str, wxString & command, wxString & param);
   wxString Join(const wxString & command, const wxString & param);

private:
   AudacityProject &mProject;

   CommandIDs mCommandMacro;
   StringArray mParamsMacro;
   bool mAbort;
   TranslatableString mMessage;

   Exporter mExporter;
   wxString mFileName;
};

#endif
