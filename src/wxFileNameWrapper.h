/**********************************************************************

Audacity: A Digital Audio Editor

wxFileNameWrapper.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_WXFILENAMEWRAPPER__
#define __AUDACITY_WXFILENAMEWRAPPER__

// The wxFileName does not have a move constructor.
// So add one to it, so that it passes around by value more quickly.
class wxFileNameWrapper : public wxFileName
{
public:
   explicit
      wxFileNameWrapper(const wxFileName &that)
      : wxFileName(that)
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
            void swap(Contents &other) {
               m_volume.swap(other.m_volume);
               m_dirs.swap(other.m_dirs);
               m_name.swap(other.m_name);
               m_ext.swap(other.m_ext);
               std::swap(m_relative, other.m_relative);
               std::swap(m_hasExt, other.m_hasExt);
               std::swap(m_dontFollowLinks, other.m_dontFollowLinks);
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

#endif

