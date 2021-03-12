/**********************************************************************

  Audacity: A Digital Audio Editor

  Languages.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_LANGUAGES__
#define __AUDACITY_LANGUAGES__

class wxArrayString;
class wxString;

#include "Audacity.h"
#include "audacity/Types.h"

AUDACITY_DLL_API
void GetLanguages(
   wxArrayString &langCodes, TranslatableStrings &langNames);

AUDACITY_DLL_API
wxString GetSystemLanguageCode();

// If no input language given, defaults first to choice in preferences, then
// to system language.
// Returns the language actually used which is not lang if lang cannot be found.
AUDACITY_DLL_API
wxString InitLang( wxString lang = {} );

// If no input language given, defaults to system language.
// Returns the language actually used which is not lang if lang cannot be found.
AUDACITY_DLL_API
wxString SetLang( const wxString & lang );

// Returns the last language code that was set
AUDACITY_DLL_API
wxString GetLang();
// Returns the last language code that was set
// Unlike GetLang, gives en rather than en_GB or en_US for result.
AUDACITY_DLL_API
wxString GetLangShort();

AUDACITY_DLL_API
wxString GetLocaleName();

#endif // __AUDACITY_LANGUAGES__
