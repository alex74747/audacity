

#include "../CommonCommandFlags.h"
#include "FileNames.h"
#include "../LabelTrack.h"
#include "../NoteTrack.h"
#include "Prefs.h"
#include "../Printing.h"
#include "Project.h"
#include "../ProjectFileIO.h"
#include "../ProjectFileManager.h"
#include "../ProjectHistory.h"
#include "../ProjectManager.h"
#include "../ProjectWindows.h"
#include "../ProjectWindow.h"
#include "../SelectFile.h"
#include "../SelectUtilities.h"
#include "../TrackPanel.h"
#include "../UndoManager.h"
#include "ViewInfo.h"
#include "../WaveTrack.h"
#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"
#include "../export/ExportMP3.h"
#include "../export/ExportMultiple.h"
#include "../import/Import.h"
#include "../import/ImportMIDI.h"
#include "../import/ImportRaw.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/FileHistory.h"
#include "../widgets/wxPanelWrapper.h"
#include "../widgets/BasicMenu.h"

#ifdef USE_MIDI
#include "../import/ImportMIDI.h"
#endif // USE_MIDI

#include <wx/app.h>

// private helper classes and functions
namespace {

void DoExport(AudacityProject &project, const FileExtension &format)
{
   auto &tracks = TrackList::Get( project );
   auto &projectFileIO = ProjectFileIO::Get( project );
   
   Exporter e{ project };

   double t0 = 0.0;
   double t1 = tracks.GetEndTime();
   auto projectName = project.GetProjectName();

   // Prompt for file name and/or extension?
   bool bPromptingRequired = !project.mBatchMode ||
                             projectName.empty() ||
                             format.empty();

   bool success = false;
   if (bPromptingRequired) {
      // Do export with prompting.
      e.SetDefaultFormat(format);
      success = e.Process(false, t0, t1);
   }
   else {
      // We either use a configured output path,
      // or we use the default documents folder - just as for exports.
      FilePath pathName = FileNames::FindDefaultPath(FileNames::Operation::MacrosOut);

      if (!FileNames::WritableLocationCheck(pathName, XO("Cannot proceed to export.")))
      {
          return;
      }
/*
      // If we've gotten to this point, we are in batch mode, have a file format,
      // and the project has either been saved or a file has been imported. So, we
      // want to use the project's path if it has been saved, otherwise use the
      // initial import path.
      FilePath pathName = !projectFileIO.IsTemporary() ?
                           wxPathOnly(projectFileIO.GetFileName()) :
                           project.GetInitialImportPath();
*/
      wxFileName fileName(pathName, projectName, format.Lower());

      // Append the "macro-output" directory to the path
      const wxString macroDir( "macro-output" );
      if (fileName.GetDirs().back() != macroDir) {
         fileName.AppendDir(macroDir);
      }

      auto justName = fileName.GetName();
      auto extension = fileName.GetExt();
      FilePath fullPath = fileName.GetFullPath();

      if (wxFileName::FileExists(fileName.GetPath())) {
         AudacityMessageBox(
            XO("Cannot create directory '%s'. \n"
               "File already exists that is not a directory"),
            Verbatim(fullPath));
         return;
      }
      fileName.Mkdir(0777, wxPATH_MKDIR_FULL); // make sure it exists

      int nChannels = (tracks.Any() - &Track::IsLeader ).empty() ? 1 : 2;

      // We're in batch mode, the file does not exist already.
      // We really can proceed without prompting.
      success = e.Process(
         nChannels,  // numChannels,
         format,     // type, 
         fullPath,   // full path,
         false,      // selectedOnly, 
         t0,         // t0
         t1          // t1
      );
   }

   if (success && !project.mBatchMode) {
      FileHistory::Global().Append(e.GetAutoExportFileName().GetFullPath());
   }
}

void DoImport(const CommandContext &context, bool isRaw)
{
   auto &project = context.project;
   auto &trackFactory = WaveTrackFactory::Get( project );
   auto &window = ProjectWindow::Get( project );

   auto selectedFiles = ProjectFileManager::ShowOpenDialog(FileNames::Operation::Import);
   if (selectedFiles.size() == 0) {
      Importer::SetLastOpenType({});
      return;
   }

   // PRL:  This affects FFmpegImportPlugin::Open which resets the preference
   // to false.  Should it also be set to true on other paths that reach
   // AudacityProject::Import ?
   NewImportingSession.Write(false);

   selectedFiles.Sort(FileNames::CompareNoCase);

   auto cleanup = finally( [&] {

      Importer::SetLastOpenType({});
      window.ZoomAfterImport(nullptr);
      window.HandleResize(); // Adjust scrollers for NEW track sizes.
   } );

   for (size_t ff = 0; ff < selectedFiles.size(); ff++) {
      auto fileName = selectedFiles[ff];

      FileNames::UpdateDefaultPath(FileNames::Operation::Import, ::wxPathOnly(fileName));

      if (isRaw) {
         TrackHolders newTracks;

         ::ImportRaw(project, &window, fileName, &trackFactory, newTracks);

         if (newTracks.size() > 0) {
            ProjectFileManager::Get( project )
               .AddImportedTracks(fileName, std::move(newTracks));
         }
      }
      else {
         ProjectFileManager::Get( project ).Import(fileName);
      }
   }
}

}

// Menu handler functions

namespace FileActions {

struct Handler : CommandHandlerObject {

void OnNew(const CommandContext & )
{
   ( void ) ProjectManager::New();
}

void OnOpen(const CommandContext &context )
{
   auto &project = context.project;
   ProjectManager::OpenFiles(&project);
}

// JKC: This is like OnClose, except it empties the project in place,
// rather than creating a new empty project (with new toolbars etc).
// It does not test for unsaved changes.
// It is not in the menus by default.  Its main purpose is/was for 
// developers checking functionality of ResetProjectToEmpty().
void OnProjectReset(const CommandContext &context)
{
   auto &project = context.project;
   ProjectManager::Get( project ).ResetProjectToEmpty();
}

void OnClose(const CommandContext &context )
{
   auto &project = context.project;
   auto &window = ProjectWindow::Get( project );
   ProjectFileManager::Get( project ).SetMenuClose(true);
   window.Close();
}

void OnCompact(const CommandContext &context)
{
   ProjectFileManager::Get(context.project).Compact();
}

void OnSave(const CommandContext &context )
{
   auto &project = context.project;
   auto &projectFileManager = ProjectFileManager::Get( project );
   projectFileManager.Save();
}

void OnSaveAs(const CommandContext &context )
{
   auto &project = context.project;
   auto &projectFileManager = ProjectFileManager::Get( project );
   projectFileManager.SaveAs();
}

void OnSaveCopy(const CommandContext &context )
{
   auto &project = context.project;
   auto &projectFileManager = ProjectFileManager::Get( project );
   projectFileManager.SaveCopy();
}

void OnExportMp3(const CommandContext &context)
{
   auto &project = context.project;
   DoExport(project, "MP3");
}

void OnExportWav(const CommandContext &context)
{
   auto &project = context.project;
   DoExport(project, "WAV");
}

void OnExportOgg(const CommandContext &context)
{
   auto &project = context.project;
   DoExport(project, "OGG");
}

void OnExportAudio(const CommandContext &context)
{
   auto &project = context.project;
   DoExport(project, "");
}

void OnExportSelection(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   Exporter e{ project };

   e.SetFileDialogTitle( XO("Export Selected Audio") );
   e.Process(true, selectedRegion.t0(),
      selectedRegion.t1());
}

void OnExportLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &window = GetProjectFrame( project );

   /* i18n-hint: filename containing exported text from label tracks */
   auto fName = _("labels.txt");
   auto trackRange = tracks.Any<const LabelTrack>();
   auto numLabelTracks = trackRange.size();

   if (numLabelTracks == 0) {
      AudacityMessageBox( XO("There are no label tracks to export.") );
      return;
   }
   else
      fName = (*trackRange.rbegin())->GetName();

   fName = SelectFile(FileNames::Operation::Export,
      XO("Export Labels As:"),
      wxEmptyString,
      fName,
      L"txt",
      { FileNames::TextFiles },
      wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
      &window);

   if (fName.empty())
      return;

   // Move existing files out of the way.  Otherwise wxTextFile will
   // append to (rather than replace) the current file.

   if (wxFileExists(fName)) {
#ifdef __WXGTK__
      auto safetyFileName = fName + L"~";
#else
      auto safetyFileName = fName + L".bak";
#endif

      if (wxFileExists(safetyFileName))
         wxRemoveFile(safetyFileName);

      wxRename(fName, safetyFileName);
   }

   wxTextFile f(fName);
   f.Create();
   f.Open();
   if (!f.IsOpened()) {
      AudacityMessageBox(
         XO( "Couldn't write to file: %s" ).Format( fName ) );
      return;
   }

   for (auto lt : trackRange)
      lt->Export(f);

   f.Write();
   f.Close();
}

void OnExportMultiple(const CommandContext &context)
{
   auto &project = context.project;
   ExportMultipleDialog em(&project);

   em.ShowModal();
}

#ifdef USE_MIDI
void OnExportMIDI(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &window = GetProjectFrame( project );

   // Make sure that there is
   // exactly one NoteTrack selected.
   const auto range = tracks.Selected< const NoteTrack >();
   const auto numNoteTracksSelected = range.size();

   if(numNoteTracksSelected > 1) {
      AudacityMessageBox(
         XO("Please select only one Note Track at a time.") );
      return;
   }
   else if(numNoteTracksSelected < 1) {
      AudacityMessageBox(
         XO("Please select a Note Track.") );
      return;
   }

   wxASSERT(numNoteTracksSelected);
   if (!numNoteTracksSelected)
      return;

   const auto nt = *range.begin();

   while(true) {

      wxString fName;

      fName = SelectFile(FileNames::Operation::Export,
         XO("Export MIDI As:"),
         wxEmptyString,
         fName,
         L"mid",
         {
            { XO("MIDI file"),    { L"mid" }, true },
            { XO("Allegro file"), { L"gro" }, true },
         },
         wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
         &window);

      if (fName.empty())
         return;

      if(!fName.Contains(L".")) {
         fName = fName + L".mid";
      }

      // Move existing files out of the way.  Otherwise wxTextFile will
      // append to (rather than replace) the current file.

      if (wxFileExists(fName)) {
#ifdef __WXGTK__
         auto safetyFileName = fName + L"~";
#else
         auto safetyFileName = fName + L".bak";
#endif

         if (wxFileExists(safetyFileName))
            wxRemoveFile(safetyFileName);

         wxRename(fName, safetyFileName);
      }

      if(fName.EndsWith(L".mid") || fName.EndsWith(L".midi")) {
         nt->ExportMIDI(fName);
      } else if(fName.EndsWith(L".gro")) {
         nt->ExportAllegro(fName);
      } else {
         auto msg = XO(
"You have selected a filename with an unrecognized file extension.\nDo you want to continue?");
         auto title = XO("Export MIDI");
         int id = AudacityMessageBox( msg, title, wxYES_NO );
         if (id == wxNO) {
            continue;
         } else if (id == wxYES) {
            nt->ExportMIDI(fName);
         }
      }
      break;
   }
}
#endif // USE_MIDI

void OnImport(const CommandContext &context)
{
   DoImport(context, false);
}

void OnImportLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &trackFactory = WaveTrackFactory::Get( project );
   auto &tracks = TrackList::Get( project );
   auto &window = ProjectWindow::Get( project );

   auto fileName =
       SelectFile(FileNames::Operation::Open,
         XO("Select a text file containing labels"),
         wxEmptyString,     // Path
         L"",       // Name
         L"txt",   // Extension
         { FileNames::TextFiles, FileNames::AllFiles },
         wxRESIZE_BORDER,        // Flags
         &window);    // Parent

   if (!fileName.empty()) {
      wxTextFile f;

      f.Open(fileName);
      if (!f.IsOpened()) {
         AudacityMessageBox(
            XO("Could not open file: %s").Format( fileName ) );
         return;
      }

      auto newTrack = std::make_shared<LabelTrack>();
      wxString sTrackName;
      wxFileName::SplitPath(fileName, NULL, NULL, &sTrackName, NULL);
      newTrack->SetName(sTrackName);

      newTrack->Import(f);

      SelectUtilities::SelectNone( project );
      newTrack->SetSelected(true);
      tracks.Add( newTrack );

      ProjectHistory::Get( project ).PushState(
         XO("Imported labels from '%s'").Format( fileName ),
            XO("Import Labels"));

      window.ZoomAfterImport(nullptr);
   }
}

#ifdef USE_MIDI
void OnImportMIDI(const CommandContext &context)
{
   auto &project = context.project;
   auto &window = GetProjectFrame( project );

   auto fileName = SelectFile(FileNames::Operation::Open,
      XO("Select a MIDI file"),
      wxEmptyString,     // Path
      L"",       // Name
      L"",       // Extension
      {
         { XO("MIDI and Allegro files"),
           { L"mid", L"midi", L"gro", }, true },
         { XO("MIDI files"),
           { L"mid", L"midi", }, true },
         { XO("Allegro files"),
           { L"gro", }, true },
         FileNames::AllFiles
      },
      wxRESIZE_BORDER,        // Flags
      &window);    // Parent

   if (!fileName.empty())
      DoImportMIDI(project, fileName);
}
#endif

void OnImportRaw(const CommandContext &context)
{
   DoImport(context, true);
}

void OnPageSetup(const CommandContext &context)
{
   auto &project = context.project;
   auto &window = GetProjectFrame( project );
   HandlePageSetup(&window);
}

void OnPrint(const CommandContext &context)
{
   auto &project = context.project;
   auto name = project.GetProjectName();
   auto &tracks = TrackList::Get( project );
   auto &window = GetProjectFrame( project );
   HandlePrint(&window, name, &tracks, TrackPanel::Get( project ));
}

void OnExit(const CommandContext &WXUNUSED(context) )
{
   // Simulate the application Exit menu item
   wxCommandEvent evt{ wxEVT_MENU, wxID_EXIT };
   wxTheApp->ProcessEvent( evt );
}

void OnExportFLAC(const CommandContext &context)
{
   DoExport(context.project, "FLAC");
}

}; // struct Handler

} // namespace

static CommandHandlerObject &findCommandHandler(AudacityProject &) {
   // Handler is not stateful.  Doesn't need a factory registered with
   // AudacityProject.
   static FileActions::Handler instance;
   return instance;
};

// Menu definitions

#define FN(X) (& FileActions::Handler :: X)

namespace {
using namespace MenuTable;

#ifdef USE_MIDI
const ReservedCommandFlag&
   NoteTracksExistFlag() { static ReservedCommandFlag flag{
      [](const AudacityProject &project){
         return !TrackList::Get( project ).Any<const NoteTrack>().empty();
      }
   }; return flag; }  //gsw
#endif

BaseItemSharedPtr FileMenu()
{
   using Options = CommandManager::Options;

   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( L"File", XXO("&File"),
      Section( "Basic",
         /*i18n-hint: "New" is an action (verb) to create a NEW project*/
         Command( L"New", XXO("&New"), FN(OnNew),
            AudioIONotBusyFlag(), L"Ctrl+N" ),

         /*i18n-hint: (verb)*/
         Command( L"Open", XXO("&Open..."), FN(OnOpen),
            AudioIONotBusyFlag(), L"Ctrl+O" ),

   #ifdef EXPERIMENTAL_RESET
         // Empty the current project and forget its name and path.  DANGEROUS
         // It's just for developers.
         // Do not translate this menu item (no XXO).
         // It MUST not be shown to regular users.
         Command( L"Reset", XXO("&Dangerous Reset..."), FN(OnProjectReset),
            AudioIONotBusyFlag() ),
   #endif

   /////////////////////////////////////////////////////////////////////////////

         Menu( L"Recent",
            {
   #ifdef __WXMAC__
               /* i18n-hint: This is the name of the menu item on Mac OS X only */
               XXO("Open Recent")
   #else
               /* i18n-hint: This is the name of the menu item on Windows and Linux */
               XXO("Recent &Files")
   #endif
               , {}, // empty accelerator

               // Bug 143 workaround.
               // For a menu that has scrollers,
               // the scrollers have an ID of 0 (not wxID_NONE which is -3).
               // Therefore wxWidgets attempts to find a help string. See
               // wxFrameBase::ShowMenuHelp(int menuId)
               // Don't find a bogus automatic help string of "Recent &Files"
               // from the submenu.
               Verbatim("")
            }
            ,
            Special( L"PopulateRecentFilesStep",
            [](AudacityProject &, BasicMenu::Handle theMenu){
               // Recent Files and Recent Projects menus
               ProjectManager::UseMenu( theMenu );
            } )
         ),

   /////////////////////////////////////////////////////////////////////////////

         Command( L"Close", XXO("&Close"), FN(OnClose),
            AudioIONotBusyFlag(), L"Ctrl+W" )
      ),

      Section( "Save",
         Menu( L"Save", XXO("&Save Project"),
            Command( L"Save", XXO("&Save Project"), FN(OnSave),
               AudioIONotBusyFlag(), L"Ctrl+S" ),
            Command( L"SaveAs", XXO("Save Project &As..."), FN(OnSaveAs),
               AudioIONotBusyFlag() ),
            Command( L"SaveCopy", XXO("&Backup Project..."), FN(OnSaveCopy),
               AudioIONotBusyFlag() )
         )//,

         // Bug 2600: Compact has interactions with undo/history that are bound
         // to confuse some users.  We don't see a way to recover useful amounts 
         // of space and not confuse users using undo.
         // As additional space used by aup3 is 50% or so, perfectly valid
         // approach to this P1 bug is to not provide the 'Compact' menu item.
         //Command( L"Compact", XXO("Co&mpact Project"), FN(OnCompact),
         //   AudioIONotBusyFlag(), L"Shift+A" )
      ),

      Section( "Import-Export",
         Menu( L"Export", XXO("&Export"),
            // Enable Export audio commands only when there are audio tracks.
            Command( L"ExportMp3", XXO("Export as MP&3"), FN(OnExportMp3),
               AudioIONotBusyFlag() | WaveTracksExistFlag() ),

            Command( L"ExportWav", XXO("Export as &WAV"), FN(OnExportWav),
               AudioIONotBusyFlag() | WaveTracksExistFlag() ),

            Command( L"ExportOgg", XXO("Export as &OGG"), FN(OnExportOgg),
               AudioIONotBusyFlag() | WaveTracksExistFlag() ),

            Command( L"Export", XXO("&Export Audio..."), FN(OnExportAudio),
               AudioIONotBusyFlag() | WaveTracksExistFlag(), L"Ctrl+Shift+E" ),

            // Enable Export Selection commands only when there's a selection.
            Command( L"ExportSel", XXO("Expo&rt Selected Audio..."),
               FN(OnExportSelection),
               AudioIONotBusyFlag() | TimeSelectedFlag() | WaveTracksSelectedFlag() ),

            Command( L"ExportLabels", XXO("Export &Labels..."),
               FN(OnExportLabels),
               AudioIONotBusyFlag() | LabelTracksExistFlag() ),
            // Enable Export audio commands only when there are audio tracks.
            Command( L"ExportMultiple", XXO("Export &Multiple..."),
               FN(OnExportMultiple),
               AudioIONotBusyFlag() | WaveTracksExistFlag(), L"Ctrl+Shift+L" )
   #if defined(USE_MIDI)
            ,
            Command( L"ExportMIDI", XXO("Export MI&DI..."), FN(OnExportMIDI),
               AudioIONotBusyFlag() | NoteTracksExistFlag() )
   #endif
         ),

         Menu( L"Import", XXO("&Import"),
            Command( L"ImportAudio", XXO("&Audio..."), FN(OnImport),
               AudioIONotBusyFlag(), L"Ctrl+Shift+I" ),
            Command( L"ImportLabels", XXO("&Labels..."), FN(OnImportLabels),
               AudioIONotBusyFlag() ),
      #ifdef USE_MIDI
            Command( L"ImportMIDI", XXO("&MIDI..."), FN(OnImportMIDI),
               AudioIONotBusyFlag() ),
      #endif // USE_MIDI
            Command( L"ImportRaw", XXO("&Raw Data..."), FN(OnImportRaw),
               AudioIONotBusyFlag() )
         )
      ),

      Section( "Print",
         Command( L"PageSetup", XXO("Pa&ge Setup..."), FN(OnPageSetup),
            AudioIONotBusyFlag() | TracksExistFlag() ),
         /* i18n-hint: (verb) It's item on a menu. */
         Command( L"Print", XXO("&Print..."), FN(OnPrint),
            AudioIONotBusyFlag() | TracksExistFlag() )
      ),

      Section( "Exit",
         // On the Mac, the Exit item doesn't actually go here...wxMac will
         // pull it out
         // and put it in the Audacity menu for us based on its ID.
         /* i18n-hint: (verb) It's item on a menu. */
         Command( L"Exit", XXO("E&xit"), FN(OnExit),
            AlwaysEnabledFlag, L"Ctrl+Q" )
      )
   ) ) };
   return menu;
}

AttachedItem sAttachment1{
   L"",
   Shared( FileMenu() )
};

BaseItemSharedPtr HiddenFileMenu()
{
   static BaseItemSharedPtr menu
   {
      (
         FinderScope{ findCommandHandler },
         ConditionalItems( L"HiddenFileItems",
            []()
            {
               // Ensures that these items never appear in a menu, but
               // are still available to scripting
               return false;
            },
            Menu( L"HiddenFileMenu", XXO("Hidden File Menu"),
               Command( L"ExportFLAC", XXO("Export as FLAC"),
                  FN(OnExportFLAC),
                  AudioIONotBusyFlag() )
            )
         )
      )
   };
   return menu;
}

AttachedItem sAttachment2{
   L"",
   Shared( HiddenFileMenu() )
};

}

#undef FN
