/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 TempDirectory.h
 
 Paul Licameli split from FileNames.h
 
 **********************************************************************/

#ifndef __AUDACITY_TEMP_DIRECTORY__
#define __AUDACITY_TEMP_DIRECTORY__


#include "Identifier.h"
class TranslatableString;
class wxWindow;

namespace TempDirectory
{
   AUDACITY_DLL_API DirectoryPath TempDir();
   AUDACITY_DLL_API void ResetTempDir();

   AUDACITY_DLL_API const DirectoryPath &DefaultTempDir();
   AUDACITY_DLL_API void SetDefaultTempDir( const DirectoryPath &tempDir );
   AUDACITY_DLL_API bool IsTempDirectoryNameOK( const DirectoryPath & Name );

   // Create a filename for an unsaved/temporary project file
   AUDACITY_DLL_API wxString UnsavedProjectFileName();

   AUDACITY_DLL_API bool FATFilesystemDenied(const FilePath &path,
                            const TranslatableString &msg,
                            wxWindow *window = nullptr);
};

#endif
