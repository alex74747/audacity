/**********************************************************************

  Audacity: A Digital Audio Editor

  Internat.h

  Markus Meyer
  Dominic Mazzoni (Mac OS X code)

**********************************************************************/

#ifndef __AUDACITY_INTERNAT__
#define __AUDACITY_INTERNAT__

#include <vector>
#include <wx/longlong.h>

#include "TranslatableString.h"

class wxArrayString;
class wxArrayStringEx;

extern STRINGS_API const wxString& GetCustomTranslation(const wxString& str1 );
extern STRINGS_API const wxString& GetCustomSubstitution(const wxString& str1 );

class LocalizedString;

// A string that can be displayed without translation, such as a technical
// acronym
//
// String manipulations, other than insertion into a format, are discouraged
//
// Do not use for proper names of people that might be transliterated to
// another alphabet
//
// Do not use for numbers because they should be formatted suitably for locale
class VerbatimString : public wxString {
public:
   friend class LocalizedString;
   VerbatimString() {}

   using wxString::empty;
   using wxString::Length;

   // Prohibit back-conversion from localized!
   explicit VerbatimString(const LocalizedString&) PROHIBITED;

   void c_str() PROHIBITED;

   explicit VerbatimString(const wxString &str) : wxString{ str } {}
   explicit VerbatimString(const wxChar *str) : wxString{ str } {}

   wxString asWxString() const { return *this; }
};

// This makes VerbatimString work as an argument in wxString::Format()
template<>
struct wxArgNormalizerNative<const VerbatimString&>
: public wxArgNormalizerNative<const wxString&>
{
   wxArgNormalizerNative(const VerbatimString& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const wxString&>( s.asWxString(), fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};

template<>
struct wxArgNormalizerNative<VerbatimString>
: public wxArgNormalizerNative<const VerbatimString&>
{
   wxArgNormalizerNative(const VerbatimString& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const VerbatimString&>( s, fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};

/* LocalizedString contains a string (or format) that HAS BEEN translated.
 * After any format substitutions, it can be shown to the user.
 * It is meant to be a short-lived object: storing it in a table is probably
 * wrong, because it will not be notified of a change in locale.
 *
 * Localized strings can be built up by substitution into formats, but
 * concatenation is discouraged.  Concatenating words and phrases may rely on
 * assumptions special to English grammar.
 *
 * Concatenation of longer localized strings, of them a sentence, is allowed
 * by use of LocalizedClause below.
 */
class LocalizedString : public wxString {
public:
   LocalizedString() {}

   LocalizedString( const wxString &string )
   : wxString{ string }
   {}

   LocalizedString( const char *chars )
   : wxString{ chars }
   {}

   LocalizedString( const wchar_t *chars )
   : wxString{ chars }
   {}

   // Assuming global locale is set, the constructors from numbers will
   // format appropriately:

   explicit
   LocalizedString( int L, const wxChar *fmt = L"%d" )
   : wxString{ wxString::Format( fmt, L ) }
   {}

   explicit
   LocalizedString( long L, const wxChar *fmt = L"%ld" )
   : wxString{ wxString::Format( fmt, L ) }
   {}

   explicit
   LocalizedString( long long LL, const wxChar *fmt = L"%lld" )
   : wxString{ wxString::Format( fmt, LL ) }
   {}

   explicit
   LocalizedString( double d, const wxChar *fmt = L"%g" )
   : wxString{ wxString::Format( fmt, d ) }
   {}

   // implicit
   LocalizedString( const VerbatimString & str )
   : wxString{ str }
   {}

   // implicit
   LocalizedString( const TranslatableString & str )
   : wxString{ str.Translation() }
   {}

   int CmpNoCase( const LocalizedString &str ) const
   { return wxString::CmpNoCase( str ); }

   friend inline bool operator ==
      ( const LocalizedString &xx, const LocalizedString &yy )
   { return (const wxString&) xx == (const wxString&) yy; }

   void c_str() PROHIBITED;
};

// This makes LocalizedString work as an argument in wxString::Format()
template<>
struct wxArgNormalizerNative<const LocalizedString&>
: public wxArgNormalizerNative<const wxString&>
{
   wxArgNormalizerNative(const LocalizedString& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const wxString&>( s, fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};

template<>
struct wxArgNormalizerNative<LocalizedString>
: public wxArgNormalizerNative<const LocalizedString&>
{
   wxArgNormalizerNative(const LocalizedString& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const LocalizedString&>( s, fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};

using LocalizedStringVector = std::vector<LocalizedString>;

/*
 * Construct localized strings of this class so that you can concatenate with
 * operator + .  Each string should represent a complete thought, not a word
 * or phrase.  Uses of this class should be few and easily reviewed.
 */
class LocalizedClause : public LocalizedString {
public:
   LocalizedClause() {}

   explicit
   LocalizedClause( const LocalizedString &string )
   : LocalizedString{ string }
   {}

   explicit
   LocalizedClause( const wxString &string )
   : LocalizedString{ string }
   {}

   explicit
   LocalizedClause( const char *chars )
   : LocalizedString{ chars }
   {}

   explicit
   LocalizedClause( const wchar_t *chars )
   : LocalizedString{ chars }
   {}

   // Permit assignment, as well as construction, from LocalizedString
   LocalizedClause &operator= (const LocalizedString &str)
   {
      if (this != &str)
         (LocalizedString&)(*this) = str;

      return *this;
   }

   LocalizedClause& operator += ( const LocalizedClause &next )
   {
      this->wxString::operator+= ( next );
      return *this;
   }

   LocalizedClause operator + ( const LocalizedClause &next )
   {
      auto result( *this );
      result += next;
      return result;
   }

   template<typename... Args>
   LocalizedClause Format(Args... args) const
   {
      return LocalizedClause {
         this->LocalizedString::Format( *this, args... )
      };
   }

};

// This makes LocalizedClause work as an argument in wxString::Format()
template<>
struct wxArgNormalizerNative<const LocalizedClause&>
: public wxArgNormalizerNative<const wxString&>
{
   wxArgNormalizerNative(const LocalizedClause& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const wxString&>( s, fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};

template<>
struct wxArgNormalizerNative<LocalizedClause>
: public wxArgNormalizerNative<const LocalizedClause&>
{
   wxArgNormalizerNative(const LocalizedClause& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const LocalizedClause&>( s, fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative & ) = delete;
   wxArgNormalizerNative &operator=( const wxArgNormalizerNative & ) = delete;
};

// Marks string for substitution only.
#define _TS( s ) GetCustomSubstitution( s )

// Marks strings for extraction only... use .Translate() to translate.
// '&', preceding menu accelerators, should NOT occur in the argument.
#define XO(s)  (TranslatableString{ (L"" s), {} })

// Alternative taking a second context argument.  A context is a string literal,
// which is not translated, but serves to disambiguate uses of the first string
// that might need differing translations, such as "Light" meaning not-heavy in
// one place but not-dark elsewhere.
#define XC(s, c)  (TranslatableString{ (L"" s), {} }.Context(c))

// Marks strings for extraction only, where '&', preceding menu accelerators, MAY
// occur.
// For now, expands exactly as macro XO does, but in future there will be a
// type distinction - for example XXO should be used for menu item names that
// might use the & character for shortcuts.
#define XXO(s)  XO(s)

// Corresponds to XC as XXO does to XO
#define XXC(s, c) XC(s, c)

#ifdef _
   #undef _
#endif

#if defined( _DEBUG )
   // Force a crash if you misuse _ in a static initializer, so that translation
   // is looked up too early and not found.

   #ifdef __WXMSW__

   // Eventually pulls in <windows.h> which indirectly defines DebugBreak(). Can't
   // include <windows.h> directly since it then causes "MemoryX.h" to spew errors.
   #include <wx/app.h>
   #define _(s) ((wxTranslations::Get() || (DebugBreak(), true)), \
                GetCustomTranslation((s)))

   #else

   #include <signal.h>
   // Raise a signal because it's even too early to use wxASSERT for this.
   #define _(s) ((wxTranslations::Get() || raise(SIGTRAP)), \
                GetCustomTranslation((s)))

   #endif

#else
   #define _(s) GetCustomTranslation((s))
#endif

#ifdef XP
   #undef XP
#endif


// The two string arguments will go to the .pot file, as
// msgid sing
// msgid_plural plural
//
// (You must use plain string literals.  Do not use _() or (L"" ) or L prefix,
//  which (intentionally) will fail to compile.  The macro inserts (L"" ) ).
//
// Note too:  The i18n-hint comment must be on the line preceding the first
// string.  That might be inside the parentheses of the macro call.
//
// The macro call is then followed by a sequence of format arguments in
// parentheses.  The third argument of the macro call is the zero-based index
// of the format argument that selects singular or plural
#define XP(sing, plur, n) \
   TranslatableString{ (L"" sing), {} }.Plural<(n)>( (L"" plur) )

// Like XP but with an additional context argument, as for XC
#define XPC(sing, plur, n, c) \
   TranslatableString{ (L"" sing), {} }.Context(c).Plural<(n)>( (L"" plur) )

class STRINGS_API Internat
{
public:
   /** \brief Initialize internationalisation support. Call this once at
    * program start. */
   static void Init();

   /** \brief Get the decimal separator for the current locale.
    *
    * Normally, this is a decimal point ('.'), but e.g. Germany uses a
    * comma (',').*/
   static wxChar GetDecimalSeparator();
   static void SetCeeNumberFormat();

   /** \brief Convert a string to a number.
    *
    * This function will accept BOTH point and comma as a decimal separator,
    * regardless of the current locale.
    * Returns 'true' on success, and 'false' if an error occurs. */
   static bool CompatibleToDouble(const wxString& stringToConvert, double* result);

   // Function version of above.
   static double CompatibleToDouble(const wxString& stringToConvert);

   /** \brief Convert a number to a string, always uses the dot as decimal
    * separator*/
   static wxString ToString(double numberToConvert,
                     int digitsAfterDecimalPoint = -1);

   /** \brief Convert a number to a string, uses the user's locale's decimal
    * separator */
   static wxString ToDisplayString(double numberToConvert,
                     int digitsAfterDecimalPoint = -1);

   /** \brief Convert a number to a string while formatting it in bytes, KB,
    * MB, GB */
   static TranslatableString FormatSize(wxLongLong size);
   static TranslatableString FormatSize(double size);

   /** \brief Check a proposed file name string for illegal characters and
    * remove them
    * return true iff name is "visibly" changed (not necessarily equivalent to
    * character-wise changed)
    */
   static bool SanitiseFilename(wxString &name, const wxString &sub);

   static const wxArrayString &GetExcludedCharacters()
   { return exclude; }

private:
   static wxChar mDecimalSeparator;

   static wxArrayString exclude;
};

// Convert C strings to wxString
#define UTF8CTOWX(X) wxString((X), wxConvUTF8)
#define LAT1CTOWX(X) wxString((X), wxConvISO8859_1)

// Whether disambiguationg contexts are supported
// If not, then the program builds and runs, but strings in the catalog with
// contexts will fail to translate
#define HAS_I18N_CONTEXTS wxCHECK_VERSION(3, 1, 1)

#endif
