#include "FileMenuCommands.h"

#include "../BatchProcessDialog.h"
#include "../Dependencies.h"
#include "../lib-src/FileDialog/FileDialog.h"
#include "../LabelTrack.h"
#include "../NoteTrack.h"
#include "../Prefs.h"
#include "../Printing.h"
#include "../Project.h"
#include "../Tags.h"
#include "../TrackPanel.h"
#include "../commands/CommandManager.h"
#include "../export/Export.h"
#include "../export/ExportMultiple.h"
#include "../import/ImportMIDI.h"
#include "../import/ImportRaw.h"
#include "../ondemand/ODManager.h"

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

   c->AddItem(wxT("CheckDeps"), _("Chec&k Dependencies..."), FN(OnCheckDependencies));

   c->AddSeparator();

   c->AddItem(wxT("EditMetaData"), _("Edit Me&tadata Tags..."), FN(OnEditMetadata));

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

   // Enable Export Selection commands only when there's a selection.
   c->AddItem(wxT("ExportSel"), _("Expo&rt Selected Audio..."), FN(OnExportSelection),
      AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag,
      AudioIONotBusyFlag | TimeSelectedFlag | WaveTracksSelectedFlag);

   c->AddItem(wxT("ExportLabels"), _("Export &Labels..."), FN(OnExportLabels),
      AudioIONotBusyFlag | LabelTracksExistFlag,
      AudioIONotBusyFlag | LabelTracksExistFlag);

   // Enable Export audio commands only when there are audio tracks.
   c->AddItem(wxT("ExportMultiple"), _("Export &Multiple..."), FN(OnExportMultiple), wxT("Ctrl+Shift+L"),
      AudioIONotBusyFlag | WaveTracksExistFlag,
      AudioIONotBusyFlag | WaveTracksExistFlag);

#if defined(USE_MIDI)
   c->AddItem(wxT("ExportMIDI"), _("Export MI&DI..."), FN(OnExportMIDI),
      AudioIONotBusyFlag | NoteTracksSelectedFlag,
      AudioIONotBusyFlag | NoteTracksSelectedFlag);
#endif

   c->AddSeparator();
   c->AddItem(wxT("ApplyChain"), _("Appl&y Chain..."), FN(OnApplyChain),
      AudioIONotBusyFlag,
      AudioIONotBusyFlag);

   c->AddItem(wxT("EditChains"), _("Edit C&hains..."), FN(OnEditChains));

   c->AddSeparator();

   c->AddItem(wxT("PageSetup"), _("Pa&ge Setup..."), FN(OnPageSetup),
      AudioIONotBusyFlag | TracksExistFlag,
      AudioIONotBusyFlag | TracksExistFlag);
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
   (void)DoEditMetadata(_("Edit Metadata Tags"), _("Metadata Tags"), true);
}

bool FileMenuCommands::DoEditMetadata
(const wxString &title, const wxString &shortUndoDescription, bool force)
{
   // Back up my tags
   auto newTags = mProject->GetTags()->Duplicate();

   if (newTags->ShowEditDialog(mProject, title, force)) {
      if (*mProject->GetTags() != *newTags) {
         // Commit the change to project state only now.
         mProject->SetTags( newTags );
         mProject->PushState(title, shortUndoDescription);
      }

      return true;
   }

   return false;
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
   ODManager::Pauser pauser;

   for (size_t ff = 0; ff < selectedFiles.GetCount(); ff++) {
      wxString fileName = selectedFiles[ff];

      wxString path = ::wxPathOnly(fileName);
      gPrefs->Write(wxT("/DefaultOpenPath"), path);

      mProject->Import(fileName);
   }

   gPrefs->Write(wxT("/LastOpenType"), wxT(""));

   gPrefs->Flush();

   mProject->HandleResize(); // Adjust scrollers for NEW track sizes.
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

      auto newTrack = mProject->GetTrackFactory()->NewLabelTrack();
      wxString sTrackName;
      wxFileName::SplitPath(fileName, NULL, NULL, &sTrackName, NULL);
      newTrack->SetName(sTrackName);

      newTrack->Import(f);

      mProject->SelectNone();
      newTrack->SetSelected(true);
      mProject->GetTracks()->Add(std::move(newTrack));

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

      DoImportMIDI(fileName);
   }
}

void FileMenuCommands::DoImportMIDI(const wxString &fileName)
{
   auto newTrack = mProject->GetTrackFactory()->NewNoteTrack();

   if (::ImportMIDI(fileName, newTrack.get())) {

      mProject->SelectNone();
      auto pTrack = mProject->GetTracks()->Add(std::move(newTrack));
      pTrack->SetSelected(true);

      mProject->PushState(wxString::Format(_("Imported MIDI from '%s'"),
         fileName.c_str()), _("Import MIDI"));

      mProject->RedrawProject();
      mProject->GetTrackPanel()->EnsureVisible(pTrack);
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

   TrackHolders newTracks;

   ::ImportRaw(mProject, fileName, mProject->GetTrackFactory(), newTracks);

   if (newTracks.size() <= 0)
      return;

   mProject->AddImportedTracks(fileName, std::move(newTracks));
   mProject->HandleResize(); // Adjust scrollers for NEW track sizes.
}

void FileMenuCommands::OnExport()
{
   Exporter e;

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);
   e.Process(mProject, false, 0.0, mProject->GetTracks()->GetEndTime());
}

void FileMenuCommands::OnExportSelection()
{
   Exporter e;

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);
   e.SetFileDialogTitle(_("Export Selected Audio"));
   e.Process(mProject, true, mProject->GetViewInfo().selectedRegion.t0(),
      mProject->GetViewInfo().selectedRegion.t1());
}

void FileMenuCommands::OnExportLabels()
{
   Track *t;
   int numLabelTracks = 0;

   TrackListIterator iter(mProject->GetTracks());

   wxString fName = _("labels.txt");
   t = iter.First();
   while (t) {
      if (t->GetKind() == Track::Label)
      {
         numLabelTracks++;
         fName = t->GetName();
      }
      t = iter.Next();
   }

   if (numLabelTracks == 0) {
      wxMessageBox(_("There are no label tracks to export."));
      return;
   }

   fName = FileSelector(_("Export Labels As:"),
               wxEmptyString,
               fName,
               wxT("txt"),
               wxT("*.txt"),
               wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
               mProject);

   if (fName == wxT(""))
      return;

   // Move existing files out of the way.  Otherwise wxTextFile will
   // append to (rather than replace) the current file.

   if (wxFileExists(fName)) {
#ifdef __WXGTK__
      wxString safetyFileName = fName + wxT("~");
#else
      wxString safetyFileName = fName + wxT(".bak");
#endif

      if (wxFileExists(safetyFileName))
         wxRemoveFile(safetyFileName);

      wxRename(fName, safetyFileName);
   }

   wxTextFile f(fName);
   f.Create();
   f.Open();
   if (!f.IsOpened()) {
      wxMessageBox(_("Couldn't write to file: ") + fName);
      return;
   }

   t = iter.First();
   while (t) {
      if (t->GetKind() == Track::Label)
         ((LabelTrack *)t)->Export(f);

      t = iter.Next();
   }

   f.Write();
   f.Close();
}

void FileMenuCommands::OnExportMultiple()
{
   ExportMultiple em(mProject);

   wxGetApp().SetMissingAliasedFileWarningShouldShow(true);
   em.ShowModal();
}

#ifdef USE_MIDI
void FileMenuCommands::OnExportMIDI(){
   TrackListIterator iter(mProject->GetTracks());
   Track *t = iter.First();
   int numNoteTracksSelected = 0;
   NoteTrack *nt = NULL;

   // Iterate through once to make sure that there is
   // exactly one NoteTrack selected.
   while (t) {
      if (t->GetSelected()) {
         if (t->GetKind() == Track::Note) {
            numNoteTracksSelected++;
            nt = (NoteTrack *)t;
         }
      }
      t = iter.Next();
   }

   if (numNoteTracksSelected > 1){
      wxMessageBox(wxString::Format(wxT(
         "Please select only one MIDI track at a time.")));
      return;
   }

   wxASSERT(nt);
   if (!nt)
      return;

   while (true){

      wxString fName = wxT("");

      fName = FileSelector(_("Export MIDI As:"),
                  wxEmptyString,
                  fName,
                  wxT(".mid|.gro"),
                  _("MIDI file (*.mid)|*.mid|Allegro file (*.gro)|*.gro"),
                  wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
                  mProject);

      if (fName == wxT(""))
         return;

      if (!fName.Contains(wxT("."))) {
         fName = fName + wxT(".mid");
      }

      // Move existing files out of the way.  Otherwise wxTextFile will
      // append to (rather than replace) the current file.

      if (wxFileExists(fName)) {
#ifdef __WXGTK__
         wxString safetyFileName = fName + wxT("~");
#else
         wxString safetyFileName = fName + wxT(".bak");
#endif

         if (wxFileExists(safetyFileName))
            wxRemoveFile(safetyFileName);

         wxRename(fName, safetyFileName);
      }

      if (fName.EndsWith(wxT(".mid")) || fName.EndsWith(wxT(".midi"))) {
         nt->ExportMIDI(fName);
      }
      else if (fName.EndsWith(wxT(".gro"))) {
         nt->ExportAllegro(fName);
      }
      else {
         wxString msg = _("You have selected a filename with an unrecognized file extension.\nDo you want to continue?");
         wxString title = _("Export MIDI");
         int id = wxMessageBox(msg, title, wxYES_NO);
         if (id == wxNO) {
            continue;
         }
         else if (id == wxYES) {
            nt->ExportMIDI(fName);
         }
      }
      break;
   }
}
#endif // USE_MIDI

void FileMenuCommands::OnApplyChain()
{
   BatchProcessDialog dlg(mProject);
   dlg.ShowModal();
   mProject->ModifyUndoMenuItems();
}

void FileMenuCommands::OnEditChains()
{
   EditChainsDialog dlg(mProject);
   dlg.ShowModal();
}

void FileMenuCommands::OnPageSetup()
{
   ::HandlePageSetup(mProject);
}
