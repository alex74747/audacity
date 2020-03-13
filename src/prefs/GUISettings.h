/**********************************************************************

Audacity: A Digital Audio Editor

@file GUISettings.h

Paul Licameli split from GUIPrefs.h

**********************************************************************/
#ifndef __AUDACITY_GUI_SETTINGS__
#define __AUDACITY_GUI_SETTINGS__

class Identifier;

namespace GUISettings {

// If no input language given, defaults to system language.
// Returns the language actually used which is not lang if lang cannot be found.
AUDACITY_DLL_API Identifier SetLang( const Identifier & lang );

}

#endif
