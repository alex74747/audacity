/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 Identifier.h
 
 Paul Licameli split from Types.h
 
 **********************************************************************/

#ifndef __AUDACITY_IDENTIFIER__
#define __AUDACITY_IDENTIFIER__

#include <vector>
#include <wx/string.h>

//! An explicitly nonlocalized string, not meant for the user to see.
/*! String manipulations are discouraged, other than splitting and joining on separator characters.
Wherever GET is used to fetch the underlying wxString, there should be a comment explaining the need for it.
 */
class STRINGS_API Identifier
{
public:
   using value_type = std::wstring;
   using char_type = value_type::value_type;

   Identifier() = default;

   //! Allow implicit conversion to this class, but not from
   Identifier(const value_type &str);

   //! Allow implicit conversion to this class, but not from
   Identifier(const wxString &str);

   // Allow implicit conversion to this class, but not from
   Identifier(const char_type *str);

   // Allow implicit conversion to this class, but not from
   Identifier(const char *str);

   // Copy construction and assignment
   Identifier( const Identifier & ) = default;
   Identifier &operator = ( const Identifier& ) = default;

   // Move construction and assignment
   Identifier( wxString && str );
   Identifier( Identifier && id )
      { swap( id ); }
   Identifier &operator= ( Identifier&& id )
   {
      if ( this != &id )
         value.clear(), swap( id );
      return *this;
   }

   void clear() { value.clear(); }

   // Implements moves
   void swap( Identifier &id ) { value.swap( id.value ); }

   //! Convenience for building concatenated identifiers.
   /*! The list must have at least two members (so you don't easily circumvent the restrictions on
    interconversions intended in TaggedIdentifier below) */
   explicit Identifier(
      std::initializer_list<Identifier> components, char_type separator);

   bool empty() const { return value.empty(); }
   size_t size() const { return value.size(); }
   size_t length() const { return value.length(); }

   //! Explicit conversion to wxString, meant to be ugly-looking and demanding of a comment why it's correct
   value_type GET() const;

   std::vector< Identifier > split( char_type separator ) const;

   static int Compare(const Identifier &a, const Identifier &b);
   static int CompareNoCase(const Identifier &a, const Identifier &b);

private:
   value_type value;
};

//! Comparisons of Identifiers are case-sensitive
bool operator == ( const Identifier &x, const Identifier &y );

inline bool operator != ( const Identifier &x, const Identifier &y )
{ return !(x == y); }

bool operator < ( const Identifier &x, const Identifier &y );

inline bool operator > ( const Identifier &x, const Identifier &y )
{ return y < x; }

inline bool operator <= ( const Identifier &x, const Identifier &y )
{ return !(y < x); }

inline bool operator >= ( const Identifier &x, const Identifier &y )
{ return !(x < y); }

namespace std
{
   template<> struct hash< Identifier > {
      size_t operator () ( const Identifier &id ) const // noexcept
      ;
   };
}

//! This lets you pass Identifier into wxFileConfig::Read
inline bool wxFromString(const wxString& str, Identifier *id)
   { if (id) { *id = str; return true; } else return false; }

//! This lets you pass Identifier into wxFileConfig::Write
wxString wxToString( const Identifier& str );

//! Template generates different TaggedIdentifier classes that don't interconvert implicitly
/*! The second template parameter determines whether comparisons are case
sensitive; default is case sensitive */
template<typename Tag, bool CaseSensitive = true >
class TaggedIdentifier : public Identifier
{
public:

   using TagType = Tag;

   using Identifier::Identifier;
   TaggedIdentifier() = default;

   //! Copy allowed only for the same Tag class and case sensitivity
   TaggedIdentifier( const TaggedIdentifier& ) = default;
   //! Move allowed only for the same Tag class and case sensitivity
   TaggedIdentifier( TaggedIdentifier&& ) = default;
   //! Copy allowed only for the same Tag class and case sensitivity
   TaggedIdentifier& operator= ( const TaggedIdentifier& ) = default;
   //! Move allowed only for the same Tag class and case sensitivity
   TaggedIdentifier& operator= ( TaggedIdentifier&& ) = default;

   //! Copy prohibited for other Tag classes or case sensitivity
   template< typename Tag2, bool b >
   TaggedIdentifier( const TaggedIdentifier<Tag2, b>& ) PROHIBITED;
   //! Move prohibited for other Tag classes or case sensitivity
   template< typename Tag2, bool b >
   TaggedIdentifier( TaggedIdentifier<Tag2, b>&& ) PROHIBITED;
   //! Copy prohibited for other Tag classes or case sensitivity
   template< typename Tag2, bool b >
   TaggedIdentifier& operator= ( const TaggedIdentifier<Tag2, b>& ) PROHIBITED;
   //! Move prohibited for other Tag classes or case sensitivity
   template< typename Tag2, bool b >
   TaggedIdentifier& operator= ( TaggedIdentifier<Tag2, b>&& ) PROHIBITED;

   /*! Allow implicit conversion to this class from un-tagged Identifier,
      but not from; resolution will use other overloads above if argument has a tag */
   TaggedIdentifier(const Identifier &str) : Identifier{ str } {}

   //! Explicit conversion to another kind of TaggedIdentifier
   template<typename String, typename = typename String::TagType>
      String CONVERT() const
         { return String{ this->GET() }; }
};

/*! Comparison of a TaggedIdentifier with an Identifier is allowed, resolving
to one of the operators on Identifiers, but always case sensitive.
Comparison operators for two TaggedIdentifiers require the same tags and case sensitivity. */
template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator == (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{
   static_assert( std::is_same< Tag1, Tag2 >::value && b1 == b2,
      "TaggedIdentifiers with different tags or sensitivity are not comparable" );
   // This test should be eliminated at compile time:
   if ( b1 )
      return Identifier::Compare(x, y) == 0;
   else
      return Identifier::CompareNoCase(x, y) == 0;
}

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator != (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{ return !(x == y); }

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator < (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{
   static_assert( std::is_same< Tag1, Tag2 >::value && b1 == b2,
      "TaggedIdentifiers with different tags or sensitivity are not comparable" );
   // This test should be eliminated at compile time:
   if ( b1 )
      return Identifier::Compare(x, y)< 0;
   else
      return Identifier::CompareNoCase(x, y) < 0;
}

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator > (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{ return y < x; }

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator <= (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{ return !(y < x); }

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator >= (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{ return !(x < y); }

namespace std
{
   template<typename Tag, bool b > struct hash< TaggedIdentifier<Tag, b> >
      : hash< Identifier > {};
}

/**************************************************************************//**

\brief type alias for identifying a Plugin supplied by a module, each module
defining its own interpretation of the strings, which may or may not be as a
file system path
********************************************************************************/
using PluginPath = wxString;
using PluginPaths = std::vector< PluginPath >;

// A key to be passed to wxConfigBase
struct RegistryPathTag;
using RegistryPath = TaggedIdentifier< RegistryPathTag >;
using RegistryPaths = std::vector< RegistryPath >;

class wxArrayStringEx;

struct CommandIdTag;
//! Identifies a menu command or macro. Case-insensitive comparison
using CommandID = TaggedIdentifier< CommandIdTag, false >;
using CommandIDs = std::vector<CommandID>;

struct PluginIdTag {};
using PluginID = TaggedIdentifier<PluginIdTag>;

// File extensions, not including any leading dot
struct FileExtensionTag;
using FileExtension = TaggedIdentifier<
   FileExtensionTag, false /* case-insensitive */
>;
using FileExtensions = std::vector< FileExtension >;

// This makes FileExtension work as an argument in wxString::Format() without
// calling GET().
// File extensions are often reported to the user in dialogs, so we permit this
// for this subclass of Identifier.
template<>
struct wxArgNormalizerNative<const FileExtension&>
: public wxArgNormalizerNative<const wxString&>
{
   wxArgNormalizerNative(const FileExtension& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const wxString&>( s.GET(), fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};
 
template<>
struct wxArgNormalizerNative<FileExtension>
: public wxArgNormalizerNative<const FileExtension&>
{
   wxArgNormalizerNative(const FileExtension& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const FileExtension&>( s.GET(), fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};

struct FilePathTag;
using FilePath = TaggedIdentifier<
   FilePathTag,

   // Case sensitivity as for wxFileName::IsCaseSensitive in filename.cpp;
   // we can't call that function but need a compile-time constant boolean as
   // the template parameter;
   // it depends on whether wxFileName::GetFormat( wxPATH_NATIVE )
   // returns wxPATH_UNIX;
   // this preprocessor logic is like what is in that function
#if defined(__WINDOWS__) || defined(__WXMAC__)
   false // case insensitive
#else
   true // case sensitive
#endif

>;
using FilePaths = std::vector< FilePath >;


// This makes FilePath work as an argument in wxString::Format() without calling
// GET().
// File paths are often reported to the user in dialogs, so we permit this
// for this subclass of Identifier.
template<>
struct wxArgNormalizerNative<const FilePath&>
: public wxArgNormalizerNative<const wxString&>
{
   wxArgNormalizerNative(const FilePath& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const wxString&>( s.GET(), fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};

template<>
struct wxArgNormalizerNative<FilePath>
: public wxArgNormalizerNative<const FilePath&>
{
   wxArgNormalizerNative(const FilePath& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const FilePath&>( s.GET(), fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};
 
#endif
