#ifndef __AUDACITY_FILE_MENU_COMMANDS__
#define __AUDACITY_FILE_MENU_COMMANDS__

#include "../Audacity.h"

class AudacityProject;
class CommandManager;
class wxString;

class FileMenuCommands
{
   FileMenuCommands(const FileMenuCommands&);
   FileMenuCommands& operator= (const FileMenuCommands&);
public:
   FileMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);

private:
   void OnNew();
public:
   void OnOpen();
private:
   void OnClose();
   void OnSave();
   void OnSaveAs();
#ifdef USE_LIBVORBIS
   void OnSaveCompressed();
#endif
   void OnCheckDependencies();
   void OnEditMetadata();

public:
   bool DoEditMetadata(const wxString &title, const wxString &shortUndoDescription, bool  force);

private:
   // Import Submenu
   void OnImport();
   void OnImportLabels();
   void OnImportMIDI();

public:
   void DoImportMIDI(const wxString &fileName);

private:
   void OnImportRaw();

   void OnExport();
   void OnExportSelection();

   AudacityProject *const mProject;
};

#endif
