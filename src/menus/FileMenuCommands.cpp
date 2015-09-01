#include "FileMenuCommands.h"

#include "../Dependencies.h"
#include "../FileDialog.h"
#include "../LabelTrack.h"
#include "../NoteTrack.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../Tags.h"
#include "../TrackPanel.h"
#include "../commands/CommandManager.h"
#include "../export/Export.h"
#include "../import/ImportMIDI.h"
#include "../import/ImportRaw.h"
#include "../ondemand/ODManager.h"

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

   c->AddSeparator();

   /////////////////////////////////////////////////////////////////////////////

   c->BeginSubMenu(_("&Import"));
   {
      c->AddItem(wxT("ImportAudio"), _("&Audio..."), FN(OnImport), wxT("Ctrl+Shift+I"));
      c->AddItem(wxT("ImportLabels"), _("&Labels..."), FN(OnImportLabels));
#ifdef USE_MIDI
      c->AddItem(wxT("ImportMIDI"), _("&MIDI..."), FN(OnImportMIDI));
#endif // USE_MIDI
      c->AddItem(wxT("ImportRaw"), _("&Raw Data..."), FN(OnImportRaw));
   }
   c->EndSubMenu();

   c->AddSeparator();

   /////////////////////////////////////////////////////////////////////////////

   // Enable Export audio commands only when there are audio tracks.
   c->AddItem(wxT("Export"), _("&Export Audio..."), FN(OnExport), wxT("Ctrl+Shift+E"),
      AudioIONotBusyFlag | WaveTracksExistFlag,
      AudioIONotBusyFlag | WaveTracksExistFlag);
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

void FileMenuCommands::OnImport()
{
   // An import trigger for the alias missing dialog might not be intuitive, but
   // this serves to track the file if the users zooms in and such.
   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);

   wxArrayString selectedFiles = AudacityProject::ShowOpenDialog(wxT(""));
   if (selectedFiles.GetCount() == 0) {
      gPrefs->Write(wxT("/LastOpenType"), wxT(""));
      gPrefs->Flush();
      return;
   }

   gPrefs->Write(wxT("/NewImportingSession"), true);

   //sort selected files by OD status.  Load non OD first so user can edit asap.
   //first sort selectedFiles.
   selectedFiles.Sort(::CompareNoCaseFileName);
   ODManager::Pause();

   for (size_t ff = 0; ff < selectedFiles.GetCount(); ff++) {
      wxString fileName = selectedFiles[ff];

      wxString path = ::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);

      mProject->Import(fileName);
   }

   gPrefs->Write(wxT("/LastOpenType"), wxT(""));

   gPrefs->Flush();

   mProject->HandleResize(); // Adjust scrollers for new track sizes.
   ODManager::Resume();
}

void FileMenuCommands::OnImportLabels()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"), ::wxGetCwd());

   wxString fileName =
      FileSelector(_("Select a text file containing labels..."),
         path,     // Path
         wxT(""),       // Name
         wxT(".txt"),   // Extension
         _("Text files (*.txt)|*.txt|All files|*"),
         wxRESIZE_BORDER,        // Flags
         mProject);    // Parent

   if (fileName != wxT("")) {
      path = ::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);
      gPrefs->Flush();

      wxTextFile f;

      f.Open(fileName);
      if (!f.IsOpened()) {
         wxMessageBox(_("Could not open file: ") + fileName);
         return;
      }

      LabelTrack *newTrack = new LabelTrack(mProject->GetDirManager());
      wxString sTrackName;
      wxFileName::SplitPath(fileName, NULL, NULL, &sTrackName, NULL);
      newTrack->SetName(sTrackName);

      newTrack->Import(f);

      mProject->SelectNone();
      mProject->GetTracks()->Add(newTrack);
      newTrack->SetSelected(true);

      mProject->PushState(wxString::
         Format(_("Imported labels from '%s'"), fileName.c_str()),
         _("Import Labels"));

      mProject->RedrawProject();
   }
}

#ifdef USE_MIDI
void FileMenuCommands::OnImportMIDI()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"), ::wxGetCwd());

   wxString fileName = FileSelector(_("Select a MIDI file..."),
         path,     // Path
         wxT(""),       // Name
         wxT(""),       // Extension
         _("MIDI and Allegro files (*.mid;*.midi;*.gro)|*.mid;*.midi;*.gro|MIDI files (*.mid;*.midi)|*.mid;*.midi|Allegro files (*.gro)|*.gro|All files|*"),
         wxRESIZE_BORDER,        // Flags
         mProject);    // Parent

   if (fileName != wxT("")) {
      path = ::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);
      gPrefs->Flush();

      NoteTrack *newTrack = new NoteTrack(mProject->GetDirManager());

      if (::ImportMIDI(fileName, newTrack)) {

         mProject->SelectNone();
         mProject->GetTracks()->Add(newTrack);
         newTrack->SetSelected(true);

         mProject->PushState(wxString::Format(_("Imported MIDI from '%s'"),
            fileName.c_str()), _("Import MIDI"));

         mProject->RedrawProject();
         mProject->GetTrackPanel()->EnsureVisible(newTrack);
      }
   }
}
#endif // USE_MIDI

void FileMenuCommands::OnImportRaw()
{
   wxString path = gPrefs->Read(wxT("/DefaultOpenPath"), ::wxGetCwd());

   wxString fileName =
      FileSelector(_("Select any uncompressed audio file..."),
      path,     // Path
      wxT(""),       // Name
      wxT(""),       // Extension
      _("All files|*"),
      wxRESIZE_BORDER,        // Flags
      mProject);    // Parent

   if (fileName == wxT(""))
      return;

   path = ::wxPathOnly(fileName);
   gPrefs->Write(wxT("/DefaultOpenPath"), path);
   gPrefs->Flush();

   Track **newTracks;
   int numTracks;

   numTracks = ::ImportRaw(mProject, fileName, mProject->GetTrackFactory(), &newTracks);

   if (numTracks <= 0)
      return;

   mProject->AddImportedTracks(fileName, newTracks, numTracks);
   mProject->HandleResize(); // Adjust scrollers for new track sizes.
}

void FileMenuCommands::OnExport()
{
   Exporter e;

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);
   e.Process(mProject, false, 0.0, mProject->GetTracks()->GetEndTime());
}
