/**********************************************************************

  Audacity: A Digital Audio Editor

  HelpText.h

  James Crook

**********************************************************************/

#ifndef __AUDACITY_HELP_TEXT__
#define __AUDACITY_HELP_TEXT__

class TranslatableString;
class wxString;

wxString HelpText( const wxString & Key );
TranslatableString TitleText( const wxString & Key );

const wxString VerCheckArgs();
const wxString VerCheckUrl();
const wxString VerCheckHtml();
wxString FormatHtmlText( const wxString & Text );

#endif
