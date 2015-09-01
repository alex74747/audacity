#include "FileMenuCommands.h"

#include "../Dependencies.h"
#include "../Project.h"
#include "../Tags.h"
#include "../commands/CommandManager.h"

#define FN(X) new ObjectCommandFunctor<FileMenuCommands>(this, &FileMenuCommands:: X )

FileMenuCommands::FileMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void FileMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(AudioIONotBusyFlag, AudioIONotBusyFlag);

   /*i18n-hint: "New" is an action (verb) to create a new project*/
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
   c->AddItem(wxT("SaveCompressed"), _("Save Compressed Copy of Project..."), FN(OnSaveCompressed));
#endif

   c->AddItem(wxT("CheckDeps"), _("Chec&k Dependencies..."), FN(OnCheckDependencies));

   c->AddSeparator();

   c->AddItem(wxT("EditMetaData"), _("Edit Me&tadata..."), FN(OnEditMetadata));
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

void FileMenuCommands::OnCheckDependencies()
{
   ::ShowDependencyDialogIfNeeded(mProject, false);
}

void FileMenuCommands::OnEditMetadata()
{
   if (mProject->GetTags()->ShowEditDialog(mProject, _("Edit Metadata Tags"), true))
      mProject->PushState(_("Edit Metadata Tags"), _("Edit Metadata"));
}
