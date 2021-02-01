/**********************************************************************

Audacity: A Digital Audio Editor

@file GUISettings.cpp

Paul Licameli split from GUIPrefs.cpp

**********************************************************************/

#include "GUISettings.h"

#include "FileNames.h"
#include "Languages.h"
#include "widgets/AudacityMessageBox.h"

#include <wx/app.h>

Identifier GUISettings::SetLang( const Identifier & lang )
{
   auto result = Languages::SetLang(FileNames::AudacityPathList(), lang);
   if (!(lang.empty() || lang == L"System") && result != lang)
      ::AudacityMessageBox(
         XO("Language \"%s\" is unknown").Format( lang.GET() ) );

#ifdef EXPERIMENTAL_CEE_NUMBERS_OPTION
   bool forceCeeNumbers;
   gPrefs->Read(L"/Locale/CeeNumberFormat", &forceCeeNumbers, false);
   if( forceCeeNumbers )
      Internat::SetCeeNumberFormat();
#endif

#ifdef __WXMAC__
      wxApp::s_macHelpMenuTitleName = _("&Help");
#endif

   return result;
}
