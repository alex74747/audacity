/**********************************************************************

  Audacity: A Digital Audio Editor

  FileNames.cpp

  James Crook

********************************************************************//**

\class FileNames
\brief Provides Static functions to yield filenames.

This class helps us with setting a base path, and makes it easier
for us to keep track of the different kinds of files we read and write
from.

JKC: In time I plan to add all file names and file extensions
used throughout Audacity into this one place.

*//********************************************************************/


#include "FileNames.h"

#include <wx/app.h>
#include <wx/defs.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/stdpaths.h>
#include "MemoryX.h"
#include "Prefs.h"
#include "Internat.h"
#include "PlatformCompatibility.h"
#include "wxFileNameWrapper.h"
#include "widgets/AudacityMessageBox.h"
#include "widgets/FileDialog/FileDialog.h"

#if defined(__WXMAC__) || defined(__WXGTK__)
#include <dlfcn.h>
#endif

#if defined(__WXMSW__)
#include <windows.h>
#endif

static DirectoryPath gDataDir;

const FileNames::FileType
     FileNames::AllFiles{ XO("All files"), { L"" } }
     /* i18n-hint an Audacity project is the state of the program, stored as
     files that can be reopened to resume the session later */
   , FileNames::AudacityProjects{ XO("AUP3 project files"), { L"aup3" }, true }
   , FileNames::DynamicLibraries{
#if defined(__WXMSW__)
      XO("Dynamically Linked Libraries"), { L"dll" }, true
#elif defined(__WXMAC__)
      XO("Dynamic Libraries"), { L"dylib" }, true
#else
      XO("Dynamically Linked Libraries"), { L"so*" }, true
#endif
     }
   , FileNames::TextFiles{ XO("Text files"), { L"txt" }, true }
   , FileNames::XMLFiles{ XO("XML files"), { L"xml", L"XML" }, true }
;

wxString FileNames::FormatWildcard( const FileTypes &fileTypes )
{
   // |-separated list of:
   // [ Description,
   //   ( if appendExtensions, then ' (', globs, ')' ),
   //   '|',
   //   globs ]
   // where globs is a ;-separated list of filename patterns, which are
   // '*' for an empty extension, else '*.' then the extension
   // Only the part before | is displayed in the choice drop-down of file
   // dialogs
   //
   // Exceptional case: if there is only one type and its description is empty,
   // then just give the globs with no |
   // Another exception: an empty description, when there is more than one
   // type, is replaced with a default
   // Another exception:  if an extension contains a dot, it is interpreted as
   // not really an extension, but a literal filename

   constexpr auto dot = L'.';
   const auto makeGlobs = [&dot]( const FileExtensions &extensions ){
      wxString globs;
      for ( const auto &ext : extensions ) {
         if ( !globs.empty() )
            globs += ';';
         const auto &extension = ext.GET();
         if ( extension.find_first_of( dot ) != std::wstring::npos )
            globs += extension;
         else {
            globs += '*';
            if ( !extension.empty() ) {
               globs += '.';
               globs += extension;
            }
         }
      }
      return globs;
   };

   const auto defaultDescription = []( const FileExtensions &extensions ){
      // Assume extensions is not empty
      auto exts = extensions[0].GET();
      for (size_t ii = 1, size = extensions.size(); ii < size; ++ii ) {
         exts += XO(", ").Translation();
         exts += extensions[ii].GET();
      }
      /* i18n-hint a type or types such as "txt" or "txt, xml" will be
         substituted for %s */
      return XO("%s files").Format( exts );
   };

   if ( fileTypes.size() == 1 && fileTypes[0].description.empty() ) {
      return makeGlobs( fileTypes[0].extensions );
   }
   else {
      wxString result;
      for ( const auto &fileType : fileTypes ) {
         const auto &extensions = fileType.extensions;
         if (extensions.empty())
            continue;

         if (!result.empty())
            result += '|';

         const auto globs = makeGlobs( extensions );

         auto mask = fileType.description;
         if ( mask.empty() )
           mask = defaultDescription( extensions );
         if ( fileType.appendExtensions )
            mask.Join( XO("(%s)").Format( globs ), " " );
         result += mask.Translation();
         result += '|';
         result += globs;
      }
      return result;
   }
}

bool FileNames::DoCopyFile(
   const FilePath& file1, const FilePath& file2, bool overwrite)
{
#ifdef __WXMSW__

   // workaround not needed
   return wxCopyFile(file1, file2, overwrite);

#else
   // PRL:  Compensate for buggy wxCopyFile that returns false success,
   // which was a cause of case 4 in comment 10 of
   // http://bugzilla.audacityteam.org/show_bug.cgi?id=1759
   // Destination file was created, but was empty
   // Bug was introduced after wxWidgets 2.8.12 at commit
   // 0597e7f977c87d107e24bf3e95ebfa3d60efc249 of wxWidgets repo

   bool existed = wxFileExists(file2);
   bool result = wxCopyFile(file1, file2, overwrite) &&
      wxFile{ file1.GET() }.Length() == wxFile{ file2.GET() }.Length();
   if (!result && !existed)
      wxRemoveFile(file2);
   return result;

#endif
}

bool FileNames::HardLinkFile( const FilePath& file1, const FilePath& file2 )
{
#ifdef __WXMSW__

   // Fix forced ASCII conversions and wrong argument order - MJB - 29/01/2019
   //return ::CreateHardLinkA( file1.c_str(), file2.c_str(), NULL );  
   return ( 0 != ::CreateHardLink( file2.GET(), file1.GET(), NULL ) );

#else

   return 0 == ::link( wxString{file1.GET()}.c_str(), wxString{file2.GET()}.c_str() );

#endif
}

DirectoryPath FileNames::MkDir(const DirectoryPath &Str)
{
   // Behaviour of wxFileName::DirExists() and wxFileName::MkDir() has
   // changed between wx2.6 and wx2.8, so we use static functions instead.
   if (!wxFileName::DirExists(Str.GET()))
      wxFileName::Mkdir(Str.GET(), 511, wxPATH_MKDIR_FULL);

   return Str;
}

// originally an ExportMultipleDialog method. Append suffix if newName appears in otherNames.
void FileNames::MakeNameUnique(FilePaths &otherNames,
   wxFileNameWrapper &newName)
{
   if ( make_iterator_range( otherNames ).index( newName.GetFullName() ) >= 0) {
      int i=2;
      wxString orig = newName.GetName();
      do {
         newName.SetName(wxString::Format(L"%s-%d", orig, i));
         i++;
      } while (
         make_iterator_range( otherNames ).index( newName.GetFullName() ) >= 0
      );
   }
   otherNames.push_back(newName.GetFullName());
}

// The APP name has upercase first letter (so that Quit Audacity is correctly
// capitalised on Mac, but we want lower case APP name in paths.
// This function does that substitution, IF the last component of
// the path is 'Audacity'.
wxString FileNames::LowerCaseAppNameInPath( const wxString & dirIn){
   wxString dir = dirIn;
   // BUG 1577 Capitalisation of Audacity in path...
   if( dir.EndsWith( "Audacity" ) )
   {
      int nChars = dir.length() - wxString( "Audacity" ).length();
      dir = dir.Left( nChars ) + "audacity";
   }
   return dir;
}

DirectoryPath FileNames::DataDir()
{
   // LLL:  Wouldn't you know that as of WX 2.6.2, there is a conflict
   //       between wxStandardPaths and wxConfig under Linux.  The latter
   //       creates a normal file as "$HOME/.audacity", while the former
   //       expects the ".audacity" portion to be a directory.
   if (gDataDir.empty())
   {
      // If there is a directory "Portable Settings" relative to the
      // executable's EXE file, the prefs are stored in there, otherwise
      // the prefs are stored in the user data dir provided by the OS.
      wxFileNameWrapper exePath{ PlatformCompatibility::GetExecutablePath() };
#if defined(__WXMAC__)
      // Path ends for example in "Audacity.app/Contents/MacOSX"
      //exePath.RemoveLastDir();
      //exePath.RemoveLastDir();
      // just remove the MacOSX part.
      exePath.RemoveLastDir();
#endif
      wxFileName portablePrefsPath(exePath.GetPath(), L"Portable Settings");

      if (::wxDirExists(portablePrefsPath.GetFullPath()))
      {
         // Use "Portable Settings" folder
         gDataDir = portablePrefsPath.GetFullPath();
      } else
      {
         // Use OS-provided user data dir folder
         wxString dataDir( LowerCaseAppNameInPath( wxStandardPaths::Get().GetUserDataDir() ));
#if defined( __WXGTK__ )
         dataDir = dataDir + L"-data";
#endif
         gDataDir = FileNames::MkDir(dataDir);
      }
   }
   return gDataDir;
}

DirectoryPath FileNames::ResourcesDir(){
   wxString resourcesDir( LowerCaseAppNameInPath( wxStandardPaths::Get().GetResourcesDir() ));
   return resourcesDir;
}

DirectoryPath FileNames::HtmlHelpDir()
{
#if defined(__WXMAC__)
   wxFileNameWrapper exePath{ PlatformCompatibility::GetExecutablePath() };
      // Path ends for example in "Audacity.app/Contents/MacOSX"
      //exePath.RemoveLastDir();
      //exePath.RemoveLastDir();
      // just remove the MacOSX part.
      exePath.RemoveLastDir();

   //for mac this puts us within the .app: Audacity.app/Contents/SharedSupport/
   return wxFileName( exePath.GetPath()+L"/help/manual", wxEmptyString ).GetFullPath();
#else
   //linux goes into /*prefix*/share/audacity/
   //windows (probably) goes into the dir containing the .exe
   wxString dataDir = FileNames::LowerCaseAppNameInPath( wxStandardPaths::Get().GetDataDir());
   return wxFileName( dataDir+L"/help/manual", wxEmptyString ).GetFullPath();
#endif
}

DirectoryPath FileNames::LegacyChainDir()
{
   // Don't force creation of it
   return wxFileNameWrapper{ DataDir(), FilePath{ L"Chains" } }.GetFullPath();
}

DirectoryPath FileNames::MacroDir()
{
   return FileNames::MkDir( wxFileNameWrapper( DataDir(), FilePath{ L"Macros" } ).GetFullPath() );
}

DirectoryPath FileNames::NRPDir()
{
   return FileNames::MkDir( wxFileNameWrapper( DataDir(), FilePath{ L"NRP" } ).GetFullPath() );
}

FilePath FileNames::NRPFile()
{
   return wxFileNameWrapper( NRPDir(), FilePath{ L"noisegate.nrp" } ).GetFullPath();
}

DirectoryPath FileNames::PlugInDir()
{
   return FileNames::MkDir( wxFileNameWrapper( DataDir(), FilePath{ L"Plug-Ins" } ).GetFullPath() );
}

FilePath FileNames::PluginRegistry()
{
   return wxFileNameWrapper( DataDir(), FilePath{ L"pluginregistry.cfg" } ).GetFullPath();
}

FilePath FileNames::PluginSettings()
{
   return wxFileNameWrapper( DataDir(), FilePath{ L"pluginsettings.cfg" } ).GetFullPath();
}

DirectoryPath FileNames::BaseDir()
{
   wxFileNameWrapper baseDir;

#if defined(__WXMAC__)
   baseDir = PlatformCompatibility::GetExecutablePath();

   // Path ends for example in "Audacity.app/Contents/MacOSX"
   //baseDir.RemoveLastDir();
   //baseDir.RemoveLastDir();
   // just remove the MacOSX part.
   baseDir.RemoveLastDir();
#elif defined(__WXMSW__)
   // Don't use wxStandardPaths::Get().GetDataDir() since it removes
   // the "Debug" directory in debug builds.
   baseDir = PlatformCompatibility::GetExecutablePath();
#else
   // Linux goes into /*prefix*/share/audacity/
   baseDir = FileNames::LowerCaseAppNameInPath(wxStandardPaths::Get().GetPluginsDir());
#endif

   return baseDir.GetPath();
}

DirectoryPath FileNames::ModulesDir()
{
   wxFileNameWrapper modulesDir{ BaseDir(), FilePath{} };

   modulesDir.AppendDir(L"modules");

   return modulesDir.GetFullPath();
}

DirectoryPath FileNames::ThemeDir()
{
   return FileNames::MkDir( wxFileNameWrapper{ DataDir(), FilePath{ L"Theme" } }.GetFullPath() );
}

DirectoryPath FileNames::ThemeComponentsDir()
{
   return FileNames::MkDir( wxFileNameWrapper{ ThemeDir(), FilePath{ L"Components" } }.GetFullPath() );
}

FilePath FileNames::ThemeCachePng()
{
   return wxFileNameWrapper{ ThemeDir(), FilePath{ L"ImageCache.png" } }.GetFullPath();
}

FilePath FileNames::ThemeCacheHtm()
{
   return wxFileNameWrapper{ ThemeDir(), FilePath{ L"ImageCache.htm" } }.GetFullPath();
}

FilePath FileNames::ThemeImageDefsAsCee()
{
   return wxFileNameWrapper{ ThemeDir(), FilePath{ L"ThemeImageDefsAsCee.h" } }.GetFullPath();
}

FilePath FileNames::ThemeCacheAsCee( )
{
// DA: Theme sourcery file name.
#ifndef EXPERIMENTAL_DA
   return wxFileNameWrapper{ ThemeDir(), FilePath{ L"ThemeAsCeeCode.h" } }.GetFullPath();
#else
   return wxFileName( ThemeDir(), FilePath{ L"DarkThemeAsCeeCode.h" } ).GetFullPath();
#endif
}

FilePath FileNames::ThemeComponent(const wxString &Str)
{
   return wxFileNameWrapper{ ThemeComponentsDir(), FilePath{ Str }, L"png" }.GetFullPath();
}

//
// Returns the full path of program module (.exe, .dll, .so, .dylib) containing address
//
FilePath FileNames::PathFromAddr(void *addr)
{
    wxFileName name;

#if defined(__WXMAC__) || defined(__WXGTK__)
   Dl_info info;
   if (dladdr(addr, &info)) {
      char realname[PLATFORM_MAX_PATH + 1];
      int len;
      name = LAT1CTOWX(info.dli_fname);
      len = readlink(OSINPUT(name.GetFullPath()), realname, PLATFORM_MAX_PATH);
      if (len > 0) {
         realname[len] = 0;
         name.SetFullName(LAT1CTOWX(realname));
      }
   }
#elif defined(__WXMSW__) && defined(_UNICODE)
   // The GetModuleHandlEx() function did not appear until Windows XP and
   // GetModuleFileName() did appear until Windows 2000, so we have to
   // check for them at runtime.
   typedef BOOL (WINAPI *getmodulehandleex)(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE* phModule);
   typedef DWORD (WINAPI *getmodulefilename)(HMODULE hModule, LPWCH lpFilename, DWORD nSize);
   getmodulehandleex gmhe =
      (getmodulehandleex) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
                                         "GetModuleHandleExW");
   getmodulefilename gmfn =
      (getmodulefilename) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),
                                         "GetModuleFileNameW");

   if (gmhe != NULL && gmfn != NULL) {
      HMODULE module;
      if (gmhe(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
               (LPTSTR) addr,
               &module)) {
         TCHAR path[MAX_PATH];
         DWORD nSize;

         nSize = gmfn(module, path, MAX_PATH);
         if (nSize && nSize < MAX_PATH) {
            name = LAT1CTOWX(path);
         }
      }
   }
#endif

    return name.GetFullPath();
}


bool FileNames::IsPathAvailable( const FilePath & Path){
   if( Path.empty() )
      return false;
#ifndef __WIN32__
   return true;
#else
   wxFileNameWrapper filePath( Path );
   return filePath.DirExists() && !filePath.FileExists();
#endif
}

wxFileNameWrapper FileNames::DefaultToDocumentsFolder(const wxString &preference)
{
   wxFileNameWrapper result;

#ifdef __WIN32__
   wxFileName defaultPath( wxStandardPaths::Get().GetDocumentsDir(), "" );
   defaultPath.AppendDir( wxTheApp->GetAppName() );
   result.SetPath( gPrefs->Read( preference, defaultPath.GetPath( wxPATH_GET_VOLUME ) ) );

   // MJB: Bug 1899 & Bug 2007.  Only create directory if the result is the default path
   bool bIsDefaultPath = result == defaultPath;
   if( !bIsDefaultPath ) 
   {
      // IF the prefs directory doesn't exist - (Deleted by our user perhaps?)
      //    or exists as a file
      // THEN fallback to using the default directory.
      bIsDefaultPath = !IsPathAvailable( result.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR ) );
      if( bIsDefaultPath )
      {
         result.SetPath( defaultPath.GetPath( wxPATH_GET_VOLUME ) );
         // Don't write to gPrefs.
         // We typically do it later, (if directory actually gets used)
      }
   }
   if ( bIsDefaultPath )
   {
      // The default path might not exist since it is a sub-directory of 'Documents'
      // There is no error if the path could not be created.  That's OK.
      // The dialog that Audacity offers will allow the user to select a valid directory.
      result.Mkdir(0755, wxPATH_MKDIR_FULL);
   }
#else
   result.AssignHomeDir();
   result.SetPath(gPrefs->Read( preference, result.GetPath() + "/Documents"));
#endif

   return result;
}

RegistryPath FileNames::PreferenceKey(
   FileNames::Operation op, FileNames::PathType type)
{
   wxString key;
   switch (op) {
      case FileNames::Operation::Temp:
         key = L"/Directories/TempDir"; break;
      case FileNames::Operation::Presets:
         key = L"/Presets/Path"; break;
      case FileNames::Operation::Open:
         key = L"/Directories/Open"; break;
      case FileNames::Operation::Save:
         key = L"/Directories/Save"; break;
      case FileNames::Operation::Import:
         key = L"/Directories/Import"; break;
      case FileNames::Operation::Export:
         key = L"/Directories/Export"; break;
      case FileNames::Operation::MacrosOut:
         key = L"/Directories/MacrosOut"; break;
      case FileNames::Operation::_None:
      default:
         break;
   }

   switch (type) {
      case FileNames::PathType::User:
         key += "/Default"; break;
      case FileNames::PathType::LastUsed:
         key += "/LastUsed"; break;
      case FileNames::PathType::_None:
      default:
         break;
   }

   return key;
}

DirectoryPath FileNames::FindDefaultPath(Operation op)
{
   auto key = PreferenceKey(op, PathType::User);

   if (key.empty())
      return wxString{};

   // If the user specified a default path, then use that
   DirectoryPath path = gPrefs->Read(key.GET(), L"");
   if (!path.empty()) {
      return path;
   }

   // Maybe the last used path is available
   key = PreferenceKey(op, PathType::LastUsed);
   path = gPrefs->Read(key.GET(), L"");
   if (!path.empty()) {
      return path;
   }

   // Last resort is to simply return the default folder
   return DefaultToDocumentsFolder("").GetPath();
}

void FileNames::UpdateDefaultPath(Operation op, const DirectoryPath &path)
{
   if (path.empty())
      return;
   RegistryPath key;
   if (op == Operation::Temp) {
      key = PreferenceKey(op, PathType::_None);
   }
   else {
      key = PreferenceKey(op, PathType::LastUsed);
   }
   if (!key.empty()) {
      gPrefs->Write(key.GET(), path);
      gPrefs->Flush();
   }
}

FilePath
FileNames::SelectFile(Operation op,
   const TranslatableString& message,
   const DirectoryPath& default_path,
   const FilePath& default_filename,
   const FileExtension& default_extension,
   const FileTypes& fileTypes,
   int flags,
   wxWindow *parent)
{
   return WithDefaultPath(op, default_path.GET(), [&](const DirectoryPath &path) {
      wxString filter;
      if ( !default_extension.empty() )
         filter = L"*." + default_extension.GET();
      return FileSelector(
            message.Translation(), path.GET(), default_filename.GET(), filter,
            FormatWildcard( fileTypes ),
            flags, parent, wxDefaultCoord, wxDefaultCoord);
   });
}

bool FileNames::IsMidi(const FilePath &fName)
{
   const FileExtension extension{ wxFileNameWrapper{ fName }.GetExt() };
   return
      extension == L"gro" ||
      extension == L"midi" ||
      extension == L"mid";
}

static DirectoryPaths sAudacityPathList;

const DirectoryPaths &FileNames::AudacityPathList()
{
   return sAudacityPathList;
}

void FileNames::SetAudacityPathList( DirectoryPaths list )
{
   sAudacityPathList = std::move( list );
}

// static
void FileNames::AddUniquePathToPathList(const DirectoryPath &pathArg,
                                          DirectoryPaths &pathList)
{
   wxFileNameWrapper pathNorm { pathArg };
   pathNorm.Normalize();

   for(const auto &path : pathList) {
      if (pathNorm == wxFileNameWrapper{ path })
         return;
   }

   pathList.push_back(pathNorm.GetFullPath());
}

// static
void FileNames::AddMultiPathsToPathList(const wxString &multiPathStringArg,
                                          DirectoryPaths &pathList)
{
   wxString multiPathString(multiPathStringArg);
   while (!multiPathString.empty()) {
      wxString onePath = multiPathString.BeforeFirst(wxPATH_SEP[0]);
      multiPathString = multiPathString.AfterFirst(wxPATH_SEP[0]);
      AddUniquePathToPathList(onePath, pathList);
   }
}

#include <wx/log.h>

// static
void FileNames::FindFilesInPathList(const wxString & pattern,
                                      const DirectoryPaths & pathList,
                                      FilePaths & results,
                                      int flags)
{
   wxLogNull nolog;

   if (pattern.empty()) {
      return;
   }

   wxFileNameWrapper ff;

   wxArrayString arrResults;
   for(size_t i = 0; i < pathList.size(); i++) {
      ff = FilePath{ { pathList[i], pattern }, wxFILE_SEP_PATH };
      wxDir::GetAllFiles(ff.GetPath(), &arrResults, ff.GetFullName(), flags);
   }
   results =
      transform_container<FilePaths>(arrResults,
         [](const wxString &str ){ return FilePath{ str }; });
}

#if defined(__WXMSW__)
static wxCharBuffer mFilename;

//
// On Windows, wxString::mb_str() can return a NULL pointer if the
// conversion to multi-byte fails.  So, based on direction intent,
// returns a pointer to an empty string or prompts for a NEW name.
//
char *FileNames::VerifyFilename(const wxString &s, bool input)
{
   static wxCharBuffer buf;
   wxString name = s;

   if (input) {
      if ((char *) (const char *)name.mb_str() == NULL) {
         name = wxEmptyString;
      }
   }
   else {
      wxFileName ff(name);
      FileExtension ext;
      while ((char *) (const char *)name.mb_str() == NULL) {
         AudacityMessageBox(
            XO(
"The specified filename could not be converted due to Unicode character use."));

         ext = ff.GetExt();
         name = FileNames::SelectFile(FileNames::Operation::_None,
            XO("Specify New Filename:"),
            wxEmptyString,
            name,
            ext,
            { ext.empty()
               ? FileNames::AllFiles
               : FileType{ {}, { ext } }
            },
            wxFD_SAVE | wxRESIZE_BORDER,
            wxGetTopLevelParent(NULL)).GET();
      }
   }

   mFilename = name.mb_str();

   return (char *) (const char *) mFilename;
}
#endif

// Using this with wxStringArray::Sort will give you a list that
// is alphabetical, without depending on case.  If you use the
// default sort, you will get strings with 'R' before 'a', because it is in caps.
int FileNames::CompareNoCase(const wxString& first, const wxString& second)
{
   return first.CmpNoCase(second);
}

// Create a unique filename using the passed prefix and suffix
FilePath FileNames::CreateUniqueName(const wxString &prefix,
                                     const wxString &suffix /* = wxEmptyString */)
{
   static int count = 0;

   return wxString::Format(L"%s %s N-%i.%s",
                           prefix,
                           wxDateTime::Now().Format(L"%Y-%m-%d %H-%M-%S"),
                           ++count,
                           suffix);
}

wxString FileNames::UnsavedProjectExtension()
{
   return L"aup3unsaved";
}

// How to detect whether the file system of a path is FAT
// No apparent way to do it with wxWidgets
#if defined(__DARWIN__)
#include <sys/mount.h>
bool FileNames::IsOnFATFileSystem(const FilePath &path)
{
   struct statfs fs;
   if (statfs(wxPathOnly(wxString{path.GET()}).c_str(), &fs))
      // Error from statfs
      return false;
   return 0 == strcmp(fs.f_fstypename, "msdos");
}
#elif defined(__linux__)
#include <sys/statfs.h>
#include "/usr/include/linux/magic.h"
bool FileNames::IsOnFATFileSystem(const FilePath &path)
{
   struct statfs fs;
   if (statfs(wxPathOnly(path.GET()).c_str(), &fs))
      // Error from statfs
      return false;
   return fs.f_type == MSDOS_SUPER_MAGIC;
}
#elif defined(_WIN32)
#include <fileapi.h>
bool FileNames::IsOnFATFileSystem(const FilePath &path)
{
   wxFileNameWrapper fileName{path};
   if (!fileName.HasVolume())
      return false;
   auto volume = AbbreviatePath(fileName) + L"\\";
   DWORD volumeFlags;
   wxChar volumeType[64];
   if (!::GetVolumeInformationW(
      volume.wc_str(), NULL, 0, NULL, NULL,
      &volumeFlags,
      volumeType,
      WXSIZEOF(volumeType)))
      return false;
   wxString type(volumeType);
   if (type == L"FAT" || type == L"FAT32")
      return true;
   return false;
}
#else
bool FileNames::IsOnFATFileSystem(const FilePath &path)
{
   return false;
}
#endif

wxString FileNames::AbbreviatePath( const wxFileNameWrapper &fileName )
{
   wxString target;
#ifdef __WXMSW__

   // Drive letter plus colon
   target = fileName.GetVolume() + L":";

#else

   // Shorten the path, arbitrarily to 3 components
   auto path = fileName;
   path.SetFullName(wxString{});
   while(path.GetDirCount() > 3)
      path.RemoveLastDir();
   target = path.GetFullPath();

#endif
   return target;
}

