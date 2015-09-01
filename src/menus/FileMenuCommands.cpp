#include "FileMenuCommands.h"
#include "../Project.h"
#include "../commands/CommandManager.h"

#define FN(X) FNT(FileMenuCommands, this, & FileMenuCommands :: X)

FileMenuCommands::FileMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void FileMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

   /*i18n-hint: "New" is an action (verb) to create a NEW project*/
   c->AddItem(wxT("New"), _("&New"), FN(OnNew), wxT("Ctrl+N"),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);
   /*i18n-hint: (verb)*/
   c->AddItem(wxT("Open"), _("&Open..."), FN(OnOpen), wxT("Ctrl+O"),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);

   /////////////////////////////////////////////////////////////////////////////

   mProject->CreateRecentFilesMenu(c);

   /////////////////////////////////////////////////////////////////////////////

   c->AddSeparator();

   c->AddItem(wxT("Close"), _("&Close"), FN(OnClose), wxT("Ctrl+W"));

   c->AddItem(wxT("Save"), _("&Save Project"), FN(OnSave), wxT("Ctrl+S"),
      AudioIONotBusyFlag | UnsavedChangesFlag,
      AudioIONotBusyFlag | UnsavedChangesFlag);
   c->AddItem(wxT("SaveAs"), _("Save Project &As..."), FN(OnSaveAs));

#ifdef USE_LIBVORBIS
   c->AddItem(wxT("SaveCompressed"), _("Sa&ve Compressed Copy of Project..."), FN(OnSaveCompressed));
#endif
}

void FileMenuCommands::OnNew()
{
   ::CreateNewAudacityProject();
}

void FileMenuCommands::OnOpen()
{
   AudacityProject::OpenFiles(mProject);
}

void FileMenuCommands::OnClose()
{
   mProject->OnClose();
}

void FileMenuCommands::OnSave()
{
   mProject->Save();
}

void FileMenuCommands::OnSaveAs()
{
   mProject->SaveAs();
}

#ifdef USE_LIBVORBIS
void FileMenuCommands::OnSaveCompressed()
{
   mProject->SaveAs(true);
}
#endif
