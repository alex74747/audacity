/**********************************************************************

Audacity: A Digital Audio Editor

wxFileNameWrapper.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_WXFILENAMEWRAPPER__
#define __AUDACITY_WXFILENAMEWRAPPER__

#include <wx/dir.h> // to inherit

#include <wx/filename.h> // to inherit
#include "Identifier.h"

// The wxFileName does not have a move constructor.
// So add one to it, so that it passes around by value more quickly.
class wxFileNameWrapper : public wxFileName
{
public:
     using wxFileName::wxFileName;

     explicit
      wxFileNameWrapper(const wxFileName &that)
      : wxFileName{ that }
   {}

   wxFileNameWrapper(const wxString &path)
      : wxFileName{ path }
   {}

   wxFileNameWrapper(const FilePath &path)
      // using GET to pass a file path to wxWidgets
      : wxFileName{ path.GET() }
   {}

   wxFileNameWrapper(const DirectoryPath &path)
      // using GET to pass a file path to wxWidgets
      : wxFileName{}
   { AssignDir( path.GET() ); }

   wxFileNameWrapper(const DirectoryPath &path, const FilePath &name)
      // using GET to pass a file path to wxWidgets
      : wxFileName{ path.GET(), name.GET() }
   {}

   wxFileNameWrapper(const DirectoryPath &path, const DirectoryPath &name)
      // using GET to pass a file path to wxWidgets
      : wxFileName{ path.GET(), name.GET() }
   {}

   wxFileNameWrapper(const DirectoryPath &path, const FilePath &name,
      const FileExtension &ext)
      // using GET to pass a file path to wxWidgets
      : wxFileName{ path.GET(), name.GET(), ext.GET() }
   {}

   wxFileNameWrapper() = default;
   wxFileNameWrapper(const wxFileNameWrapper &that) = default;
   wxFileNameWrapper &operator= (const wxFileNameWrapper &that) = default;

   void swap(wxFileNameWrapper &that)
   {
      if (this != &that) {
#if 0
         // Awful hack number 1 makes gcc 5 choke
         enum : size_t { Size = sizeof(*this) };
         // Do it bitwise.
         // std::aligned_storage<Size>::type buffer;
         char buffer[Size];
         memcpy(&buffer, this, Size);
         memcpy(this, &that, Size);
         memcpy(&that, &buffer, Size);
#else
         // Awful hack number 2 relies on knowing the class layout
         // This is the less evil one but watch out for redefinition of the base class
         struct Contents
         {
            void swap(Contents &that) {
               m_volume.swap(that.m_volume);
               m_dirs.swap(that.m_dirs);
               m_name.swap(that.m_name);
               m_ext.swap(that.m_ext);
               std::swap(m_relative, that.m_relative);
               std::swap(m_hasExt, that.m_hasExt);
               std::swap(m_dontFollowLinks, that.m_dontFollowLinks);
            };

            wxString        m_volume;
            wxArrayString   m_dirs;
            wxString        m_name, m_ext;
            bool            m_relative;
            bool            m_hasExt;
            bool            m_dontFollowLinks;
         };

         reinterpret_cast<Contents*>(this)->swap
            (*reinterpret_cast<Contents*>(&that));
#endif
      }
   }

   // Define move copy and assignment in terms of swap
   wxFileNameWrapper(wxFileNameWrapper &&that)
   {
      swap(that);
   }

   wxFileNameWrapper &operator= (wxFileNameWrapper &&that)
   {
      if (this != &that) {
         Clear();
         swap(that);
      }
      return *this;
   }
};

class wxDirWrapper : public wxDir
{
public:
   using wxDir::wxDir;
   wxDirWrapper( const DirectoryPath &path ) : wxDir{ path.GET() } {}

    // simplest version of Traverse(): get the names of all files under this
    // directory into filenames array, return the number of files
    static inline size_t GetAllFiles(const DirectoryPath& dirname,
                              FilePaths *files,
                              const wxString& filespec = wxEmptyString,
                              int flags = wxDIR_DEFAULT)
   {
      wxArrayString intermediate;
      auto result = wxDir::GetAllFiles(
         dirname.GET(),
         files ? &intermediate : nullptr,
         filespec,
         flags
      );
      if ( files )
         *files = { intermediate.begin(), intermediate.end() };
      return result;
   }
};

#endif

