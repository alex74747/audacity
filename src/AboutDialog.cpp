/**********************************************************************

  Audacity: A Digital Audio Editor

  AboutDialog.cpp

  Dominic Mazzoni
  Vaughan Johnson
  James Crook

********************************************************************//**

\class AboutDialog
\brief The AboutDialog shows the program version and developer credits.

It is a simple scrolling window with an 'OK... Audacious!' button to
close it.

*//*****************************************************************//**

\class AboutDialogCreditItem
\brief AboutDialogCreditItem is a structure used by the AboutDialog to
hold information about one contributor to Audacity.

*//********************************************************************/



#include "AboutDialog.h"

#include <wx/dialog.h>
#include <wx/html/htmlwin.h>
#include <wx/button.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/intl.h>
#include <wx/sstream.h>
#include <wx/txtstrm.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include "FileNames.h"
#include "HelpText.h"
#include "ShuttleGui.h"
#include "widgets/HelpSystem.h"
#include "ui/AccessibleLinksFormatter.h"

#include "AllThemeResources.h"
#include "Theme.h"

// DA: Logo for About box.
#ifdef EXPERIMENTAL_DA
#include "../images/DarkAudacityLogoWithName.xpm"
#else
#include "../images/AudacityLogoWithName.xpm"
#endif

// Notice this is a "system include".  This is on purpose and only until
// we convert over to CMake.  Once converted, the "RevisionIndent.h" file
// should be deleted and this can be changed back to a user include if
// desired.
//
// RevisionIdent.h may contain #defines like these ones:
//#define REV_LONG "28864acb238cb3ca71dda190a2d93242591dd80e"
//#define REV_TIME "Sun Apr 12 12:40:22 2015 +0100"
#include "RevisionIdent.h"

#ifndef REV_TIME
#define REV_TIME "unknown date and time"
#endif

#ifdef REV_LONG
#define REV_IDENT wxString( "[[https://github.com/audacity/audacity/commit/" )+ REV_LONG + "|" + wxString( REV_LONG ).Left(6) + "]] of " +  REV_TIME 
#else
#define REV_IDENT (XO("No revision identifier was provided").Translation())
#endif

#if defined(HAS_SENTRY_REPORTING) || defined(HAVE_UPDATES_CHECK) || defined(USE_BREAKPAD)
#define HAS_PRIVACY_POLICY
#endif

// To substitute into many other translatable strings
static const auto ProgramName =
   //XO("Audacity");
   Verbatim("Audacity");

void AboutDialog::CreateCreditsList()
{
   const auto sysAdminFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, system administration");
   const auto coFounderFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, co-founder and developer");
  const auto designerFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, designer");
   const auto developerFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, developer");
   const auto developerAndSupprtFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, developer and support");
   const auto documentationAndSupportFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, documentation and support");
   const auto qaDocumentationAndSupportFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, QA tester, documentation and support");
   const auto documentationAndSupportFrenchFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, documentation and support, French");
   const auto qualityAssuranceFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, quality assurance");
   const auto accessibilityAdvisorFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, accessibility advisor");
   const auto graphicArtistFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, graphic artist");
   const auto composerFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, composer");
   const auto testerFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, tester");
   const auto NyquistPluginsFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, Nyquist plug-ins");
   const auto webDeveloperFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, web developer");
   const auto graphicsFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, graphics");

   // The Audacity Team: developers and support

   AddCredit(L"Peter Jonas", developerFormat, roleTeamMember);
   AddCredit(L"Martin Keary", roleTeamMember);
   AddCredit(L"Paul Licameli", developerFormat, roleTeamMember);
   AddCredit(L"Pavel Penikov", testerFormat, roleTeamMember);
   AddCredit(L"Anita Sudan", roleTeamMember);
   AddCredit(L"Vitaly Sverchinsky", developerFormat, roleTeamMember);
   AddCredit(L"Dmitry Vedenko", developerFormat, roleTeamMember);
   AddCredit(L"Leo Wattenberg", documentationAndSupportFormat, roleTeamMember);
   
   // Former Musers
   AddCredit(L"Anton Gerasimov", developerFormat, roleExMuse);
   AddCredit(L"Jouni Helminen", designerFormat, roleExMuse);
   
   // Emeritus: people who were "lead developers" or made an
   // otherwise distinguished contribution, but who are no
   // longer active.
   AddCredit(
      L"[[https://wiki.audacityteam.org/wiki/User:Galeandrews|Gale Andrews]]",
      qualityAssuranceFormat, roleEmeritusTeam);
   AddCredit(L"Richard Ash", developerFormat, roleEmeritusTeam);
   AddCredit(L"Christian Brochec",
      documentationAndSupportFrenchFormat, roleEmeritusTeam);
   AddCredit(L"Matt Brubeck", developerFormat, roleEmeritusTeam);
   AddCredit(L"Arturo \"Buanzo\" Busleiman", sysAdminFormat, roleEmeritusTeam);
   AddCredit(L"Michael Chinen", developerFormat, roleEmeritusTeam);
   AddCredit(L"James Crook", developerFormat, roleEmeritusTeam);
   AddCredit(L"Roger Dannenberg", coFounderFormat, roleEmeritusTeam);
   AddCredit(L"Steve Daulton", roleEmeritusTeam);
   AddCredit(L"Al Dimond", developerFormat, roleEmeritusTeam);
   AddCredit(L"Benjamin Drung", developerFormat, roleEmeritusTeam);
   AddCredit(L"Joshua Haberman", developerFormat, roleEmeritusTeam);
   AddCredit(L"Ruslan Ijbulatov", developerFormat, roleEmeritusTeam);
   AddCredit(L"Vaughan Johnson", developerFormat, roleEmeritusTeam);
   AddCredit(L"Greg Kozikowski", documentationAndSupportFormat, roleEmeritusTeam);
   AddCredit(L"Leland Lucius", developerFormat, roleEmeritusTeam);
   AddCredit(L"Dominic Mazzoni", coFounderFormat, roleEmeritusTeam);
   AddCredit(L"Markus Meyer", developerFormat, roleEmeritusTeam);
   AddCredit(L"Monty Montgomery", developerFormat, roleEmeritusTeam);
   AddCredit(L"Shane Mueller", developerFormat, roleEmeritusTeam);
   AddCredit(L"Tony Oetzmann", documentationAndSupportFormat, roleEmeritusTeam);
   AddCredit(L"Alexandre Prokoudine", documentationAndSupportFormat, roleEmeritusTeam);
   AddCredit(L"Peter Sampson", qaDocumentationAndSupportFormat, roleEmeritusTeam);
   AddCredit(L"Martyn Shaw", developerFormat, roleEmeritusTeam);
   AddCredit(L"Bill Wharrie", documentationAndSupportFormat, roleEmeritusTeam);

   // Contributors
   AddCredit(L"Lynn Allan", developerFormat, roleContributor);
   AddCredit(L"Brian Armstrong", developerFormat, roleContributor);
   AddCredit(L"David Avery", developerFormat, roleContributor);
   AddCredit(L"David Bailes", accessibilityAdvisorFormat, roleContributor);
   AddCredit(L"William Bland", developerFormat, roleContributor);
   AddCredit(L"Sami Boukortt", developerFormat, roleContributor);
   AddCredit(L"Jeremy R. Brown", developerFormat, roleContributor);
   AddCredit(L"Alex S. Brown", developerFormat, roleContributor);
   AddCredit(L"Chris Cannam", developerFormat, roleContributor);
   AddCredit(L"Cory Cook", developerFormat, roleContributor);
   AddCredit(L"Craig DeForest", developerFormat, roleContributor);
   AddCredit(L"Edgar Franke (Edgar-RFT)", developerFormat, roleContributor);
   AddCredit(L"Mitch Golden", developerFormat, roleContributor);
   AddCredit(L"Brian Gunlogson", developerFormat, roleContributor);
   AddCredit(L"Andrew Hallendorff", developerFormat, roleContributor);
   AddCredit(L"Robert H\u00E4nggi", developerFormat, roleContributor);
   AddCredit(L"Daniel Horgan", developerFormat, roleContributor);
   AddCredit(L"David Hostetler", developerFormat, roleContributor);
   AddCredit(L"Edward Hui", developerFormat, roleContributor);
   AddCredit(L"Steve Jolly", developerFormat, roleContributor);
   AddCredit(L"Steven Jones", developerFormat, roleContributor);
   AddCredit(L"Henric Jungheim", developerFormat, roleContributor);
   AddCredit(L"Myungchul Keum", developerFormat, roleContributor);
   AddCredit(L"Arun Kishore", developerFormat, roleContributor);
   AddCredit(L"Paul Livesey", developerFormat, roleContributor);
   AddCredit(L"Harvey Lubin", graphicArtistFormat, roleContributor);
   AddCredit(L"Max Maisel", developerFormat, roleContributor);
   AddCredit(L"Greg Mekkes", developerFormat, roleContributor);
   AddCredit(L"Abe Milde", developerFormat, roleContributor);
   AddCredit(L"Paul Nasca", developerFormat, roleContributor);
   AddCredit(L"Clayton Otey", developerFormat, roleContributor);
   AddCredit(L"Mark Phillips", developerFormat, roleContributor);
   AddCredit(L"Andr\u00E9 Pinto", developerFormat, roleContributor);
   AddCredit(L"Jean Claude Risset", composerFormat, roleContributor);
   AddCredit(L"Augustus Saunders", developerFormat, roleContributor);
   AddCredit(L"Benjamin Schwartz", developerFormat, roleContributor);
   AddCredit(L"Cliff Scott", testerFormat, roleContributor);
   AddCredit(L"David R. Sky", NyquistPluginsFormat, roleContributor);
   AddCredit(L"Rob Sykes", developerFormat, roleContributor);
   AddCredit(L"Mike Underwood", developerFormat, roleContributor);
   AddCredit(L"Philip Van Baren", developerFormat, roleContributor);
   AddCredit(L"Salvo Ventura", developerFormat, roleContributor);
   AddCredit(L"Darrell Walisser", developerFormat, roleContributor);
   AddCredit(L"Jun Wan", developerFormat, roleContributor);
   AddCredit(L"Daniel Winzen", developerFormat, roleContributor);
   AddCredit(L"Tom Woodhams", developerFormat, roleContributor);
   AddCredit(L"Mark Young", developerFormat, roleContributor);
   AddCredit(L"Wing Yu", developerFormat, roleContributor);

   // Website and Graphics
   AddCredit(L"Shinta Carolinasari", webDeveloperFormat, roleGraphics);
   AddCredit(L"Bayu Rizaldhan Rayes", graphicsFormat, roleGraphics);

   // Libraries

   AddCredit(L"[[https://libexpat.github.io/|expat]]", roleLibrary);
   AddCredit(L"[[https://xiph.org/flac/|FLAC]]", roleLibrary);
   AddCredit(L"[[http://lame.sourceforge.net/|LAME]]", roleLibrary);
   AddCredit(L"[[https://www.underbit.com/products/mad/|libmad]]", roleLibrary);
   AddCredit(L"[[http://www.mega-nerd.com/libsndfile/|libsndfile]]", roleLibrary);
   AddCredit(L"[[https://sourceforge.net/p/soxr/wiki/Home/|libsoxr]]", roleLibrary);
   AddCredit(
      XO("%s (incorporating %s, %s, %s, %s and %s)")
         .Format(
            "[[http://lv2plug.in/|lv2]]",
            "lilv",
            "msinttypes",
            "serd",
            "sord",
            "sratom"
         ).Translation(),
      roleLibrary);
   AddCredit(L"[[https://www.cs.cmu.edu/~music/nyquist/|Nyquist]]", roleLibrary);
   AddCredit(L"[[https://xiph.org/vorbis/|Ogg Vorbis]]", roleLibrary);
   AddCredit(L"[[http://www.portaudio.com/|PortAudio]]", roleLibrary);
   AddCredit(L"[[http://www.portmedia.sourceforge.net/portmidi/|PortMidi]]", roleLibrary);
   AddCredit(L"[[https://sourceforge.net/p/portmedia/wiki/portsmf/|portsmf]]", roleLibrary);
   AddCredit(L"[[http://sbsms.sourceforge.net/|sbsms]]", roleLibrary);
   AddCredit(L"[[https://www.surina.net/soundtouch/|SoundTouch]]", roleLibrary);
   AddCredit(L"[[http://www.twolame.org/|TwoLAME]]", roleLibrary);
   AddCredit(L"[[http://www.vamp-plugins.org/|Vamp]]", roleLibrary);
   AddCredit(L"[[https://wxwidgets.org/|wxWidgets]]", roleLibrary);

   // Thanks

   AddCredit(L"Dave Beydler", roleThanks);
   AddCredit(L"Brian Cameron", roleThanks);
   AddCredit(L"Jason Cohen", roleThanks);
   AddCredit(L"Dave Fancella", roleThanks);
   AddCredit(L"Steve Harris", roleThanks);
   AddCredit(L"Daniel James", roleThanks);
   AddCredit(L"Daniil Kolpakov", roleThanks);
   AddCredit(L"Robert Leidle", roleThanks);
   AddCredit(L"Logan Lewis", roleThanks);
   AddCredit(L"David Luff", roleThanks);
   AddCredit(L"Jason Pepas", roleThanks);
   AddCredit(L"Jonathan Ryshpan", roleThanks);
   AddCredit(L"Michael Schwendt", roleThanks);
   AddCredit(L"Patrick Shirkey", roleThanks);
   AddCredit(L"Tuomas Suutari", roleThanks);
   AddCredit(L"Mark Tomlinson", roleThanks);
   AddCredit(L"David Topper", roleThanks);
   AddCredit(L"Rudy Trubitt", roleThanks);
   AddCredit(L"StreetIQ.com", roleThanks);
   AddCredit(L"UmixIt Technologies, LLC", roleThanks);
   AddCredit(L"Verilogix, Inc.", roleThanks);
}

// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(AboutDialog, wxDialogWrapper)
   EVT_BUTTON(wxID_OK, AboutDialog::OnOK)
END_EVENT_TABLE()

IMPLEMENT_CLASS(AboutDialog, wxDialogWrapper)

namespace {
   AboutDialog *sActiveInstance{};
}

AboutDialog *AboutDialog::ActiveIntance()
{
   return sActiveInstance;
}

AboutDialog::AboutDialog(wxWindow * parent)
   /* i18n-hint: information about the program */
   :  wxDialogWrapper(parent, -1, XO("About %s").Format( ProgramName ),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
   wxASSERT(!sActiveInstance);
   sActiveInstance = this;

   SetName();
   this->SetBackgroundColour(theTheme.Colour( clrAboutBoxBackground ));
   //this->SetBackgroundColour(theTheme.Colour( clrMedium ));
   icon = NULL;
   ShuttleGui S( this, eIsCreating );
   S.StartNotebook();
   {
      PopulateAudacityPage( S );
      PopulateInformationPage( S );
      PopulateLicensePage( S );
   }
   S.EndNotebook();

   S.Id(wxID_OK)
      .Prop(0)
      .AddButton(XXO("OK"), wxALIGN_CENTER, true);

   Layout();
   Fit();
   this->Centre();
}

#define ABOUT_DIALOG_WIDTH 506

void AboutDialog::PopulateAudacityPage( ShuttleGui & S )
{
   CreateCreditsList();


   /* i18n-hint: The translation of "translator_credits" will appear
    *  in the credits in the About Audacity window.  Use this to add
    *  your own name(s) to the credits.
    *
    *  For example:  "English translation by Dominic Mazzoni." */
   auto translatorCreditsMsgid = XO("translator_credits");
   auto translatorCredits = translatorCreditsMsgid.Translation();
   if ( translatorCredits == translatorCreditsMsgid.MSGID().GET() )
      // We're in an English locale
      translatorCredits.clear();
   else
      translatorCredits += L"<br>";

   wxStringOutputStream o;
   wxTextOutputStream informationStr( o );   // string to build up list of information in
   informationStr
      << L"<center>"
// DA: Description and provenance in About box
#ifdef EXPERIMENTAL_DA
      #undef _
      #define _(s) wxGetTranslation((s))
      << L"<h3>DarkAudacity "
      << wxString(AUDACITY_VERSION_STRING)
      << L"</center></h3>"
      << L"Customised version of the Audacity free, open source, cross-platform software "
      << L"for recording and editing sounds."
      << L"<p><br>&nbsp; &nbsp; <b>Audacity<sup>&reg;</sup></b> software is copyright &copy; 1999-2021 Audacity Team.<br>"
      << L"&nbsp; &nbsp; The name <b>Audacity</b> is a registered trademark.<br><br>"

#else
      << XO("<h3>")
      << ProgramName
      << L" "
      << wxString(AUDACITY_VERSION_STRING)
      << L"</center></h3>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("%s the free, open source, cross-platform software for recording and editing sounds.")
            .Format(ProgramName)
#endif

      << L"<h3>"
      << XO("Credits")
      << L"</h3>"
      << L"<p>"

// DA: Customisation credit
#ifdef EXPERIMENTAL_DA
      << L"<p><b>"
      << XO("DarkAudacity Customisation")
      << L"</b><br>"
      << L"James Crook, art, coding &amp; design<br>"
#endif

      << L"<p><b>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("%s Team Members").Format( ProgramName )
      << L"</b><br>"
      << GetCreditsByRole(roleTeamMember)
	  
	  << L"<p><b>"
      << XO("Former Musers")
      << L"</b><br>"
      << GetCreditsByRole(roleExMuse)

      << L"<p><b> "
      << XO("Emeritus:")
      << L"</b><br>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("Distinguished %s Team members, not currently active")
         .Format( ProgramName )
      << L"<br><br>"
      << GetCreditsByRole(roleEmeritusTeam)

      << L"<p><b>"
      << XO("Contributors")
      << L"</b><br>"
      << GetCreditsByRole(roleContributor)

      << L"<p><b>"
      << XO("Website and Graphics")
      << L"</b><br>"
      << GetCreditsByRole(roleGraphics)
   ;

   if(!translatorCredits.empty()) informationStr
      << L"<p><b>"
      << XO("Translators")
      << L"</b><br>"
      << translatorCredits
   ;

   informationStr
      << L"<p><b>"
      << XO("Libraries")
      << L"</b><br>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("%s includes code from the following projects:").Format( ProgramName )
      << L"<br><br>"
      << GetCreditsByRole(roleLibrary)

      << L"<p><b>"
      << XO("Special thanks:")
      << L"</b><br>"
      << GetCreditsByRole(roleThanks)

      << L"<p><br>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("%s website: ").Format( ProgramName )
      << L"[[https://www.audacityteam.org/|https://www.audacityteam.org/]]"

// DA: Link for DA url too
#ifdef EXPERIMENTAL_DA
      << L"<br>DarkAudacity website: [[http://www.darkaudacity.com/|https://www.darkaudacity.com/]]"
#else
      << L"<p><br>&nbsp; &nbsp; "
      /* i18n-hint Audacity's name substitutes for first and third %s,
       and a "copyright" symbol for the second */
      << XO("%s software is copyright %s 1999-2021 %s Team.")
         .Format(
            Verbatim("<b>%s<sup>&reg;</sup></b>").Format( ProgramName ),
            L"&copy;",
            ProgramName )
      << L"<br>"

      << L"&nbsp; &nbsp; "
      /* i18n-hint Audacity's name substitutes for %s */
      << XO("The name %s is a registered trademark.")
         .Format( Verbatim("<b>%s</b>").Format( ProgramName ) )
      << L"<br><br>"
#endif

      << L"</center>"
   ;

   auto pPage = S.StartNotebookPage( ProgramName );
   S.StartVerticalLay(1);
   {
      //v For now, change to AudacityLogoWithName via old-fashioned way, not Theme.
      wxBitmap logo(AudacityLogoWithName_xpm); //v

      // JKC: Resize to 50% of size.  Later we may use a smaller xpm as
      // our source, but this allows us to tweak the size - if we want to.
      // It also makes it easier to revert to full size if we decide to.
      const float fScale = 0.5f;// smaller size.
      wxImage RescaledImage(logo.ConvertToImage());
      wxColour MainColour(
         RescaledImage.GetRed(1,1),
         RescaledImage.GetGreen(1,1),
         RescaledImage.GetBlue(1,1));
      pPage->SetBackgroundColour(MainColour);
      // wxIMAGE_QUALITY_HIGH not supported by wxWidgets 2.6.1, or we would use it here.
      RescaledImage.Rescale((int)(LOGOWITHNAME_WIDTH * fScale), (int)(LOGOWITHNAME_HEIGHT *fScale));
      wxBitmap RescaledBitmap(RescaledImage);

      icon =
         safenew wxStaticBitmap(S.GetParent(), -1,
         //*logo, //v
         //v theTheme.Bitmap(bmpAudacityLogo), wxPoint(93, 10), wxSize(215, 190));
         //v theTheme.Bitmap(bmpAudacityLogoWithName),
         RescaledBitmap,
         wxDefaultPosition,
         wxSize((int)(LOGOWITHNAME_WIDTH*fScale), (int)(LOGOWITHNAME_HEIGHT*fScale)));
   }
   S.Prop(0).AddWindow( icon );

   HtmlWindow *html = safenew LinkingHtmlWindow(S.GetParent(), -1,
                                         wxDefaultPosition,
                                         wxSize(ABOUT_DIALOG_WIDTH, 359),
                                         wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);
   html->SetPage( FormatHtmlText( o.GetString() ) );

   /* locate the html renderer where it fits in the dialogue */
   S.Prop(1).Position( wxEXPAND ).Focus()
      .AddWindow( html );

   S.EndVerticalLay();
   S.EndNotebookPage();
}


/** \brief: Fills out the "Information" tab of the preferences dialogue
 *
 * Provides as much information as possible about build-time options and
 * the libraries used, to try and make Linux support easier. Basically anything
 * about the build we might wish to know should be visible here */
void AboutDialog::PopulateInformationPage( ShuttleGui & S )
{
   wxStringOutputStream o;
   wxTextOutputStream informationStr( o );   // string to build up list of information in
   S.StartNotebookPage( XO("Build Information") );  // start the tab
   S.StartVerticalLay(2);  // create the window
   HtmlWindow *html = safenew LinkingHtmlWindow(S.GetParent(), -1, wxDefaultPosition,
                           wxSize(ABOUT_DIALOG_WIDTH, 264),
                           wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);
   // create a html pane in it to put the content in.
   auto enabled = XO("Enabled");
   auto disabled = XO("Disabled");
   wxString blank;

   /* this builds up the list of information to go in the window in the string
    * informationStr */
   informationStr
      << L"<h2><center>"
      << XO("Build Information")
      << L"</center></h2>\n"
      << VerCheckHtml();
 
   informationStr
      << L"<h3>"
   /* i18n-hint: Information about when audacity was compiled follows */
      << XO("The Build")
      << L"</h3>\n<table>"; // start build info table

   // Current date
   AddBuildinfoRow(&informationStr, XO("Program build date:"), __TDATE__);
   AddBuildinfoRow(&informationStr, XO("Commit Id:"), REV_IDENT );

   auto buildType =
#ifdef _DEBUG
      XO("Debug build (debug level %d)").Format(wxDEBUG_LEVEL);
#else
      XO("Release build (debug level %d)").Format(wxDEBUG_LEVEL);
#endif
   ;
   if( (sizeof(void*) == 8) )
      buildType = XO("%s, 64 bits").Format( buildType );

// Remove this once the transition to CMake is complete
#if defined(CMAKE)
   buildType = Verbatim("CMake %s").Format( buildType );
#endif

   AddBuildinfoRow(&informationStr, XO("Build type:"), buildType.Translation());

#ifdef _MSC_FULL_VER
   AddBuildinfoRow(&informationStr, XO("Compiler:"),
	   wxString::Format(L"MSVC %02d.%02d.%05d.%02d", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 100000, _MSC_BUILD));
#endif

#ifdef __GNUC_PATCHLEVEL__
#ifdef __MINGW32__
   AddBuildinfoRow(&informationStr, XO("Compiler:"), L"MinGW " wxMAKE_VERSION_DOT_STRING_T(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__));
#else
   AddBuildinfoRow(&informationStr, XO("Compiler:"), L"GCC " wxMAKE_VERSION_DOT_STRING_T(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__));
#endif
#endif

#ifdef __clang_version__
   AddBuildinfoRow(&informationStr, XO("Compiler:"), L"clang " __clang_version__);
#endif

   // Install prefix
#ifdef __WXGTK__
   /* i18n-hint: The directory audacity is installed into (on *nix systems) */
   AddBuildinfoRow(&informationStr, XO("Installation Prefix:"), \
         wxT(INSTALL_PREFIX));
#endif

   // Location of settings
   AddBuildinfoRow(&informationStr, XO("Settings folder:"), \
      FileNames::DataDir());

   informationStr << L"</table>\n"; // end of build info table


   informationStr
      << L"<h3>"
      /* i18n-hint: Libraries that are essential to audacity */
      << XO("Core Libraries")
      << L"</h3>\n<table>";  // start table of core libraries

   AddBuildinfoRow(&informationStr, L"wxWidgets",
         XO("Cross-platform GUI library"), Verbatim(wxVERSION_NUM_DOT_STRING_T));

   AddBuildinfoRow(&informationStr, L"PortAudio",
         XO("Audio playback and recording"), Verbatim(L"v19"));

   AddBuildinfoRow(&informationStr, L"libsoxr",
         XO("Sample rate conversion"), enabled);

   informationStr << L"</table>\n"; // end table of core libraries

   informationStr
      << L"<h3>"
      << XO("File Format Support")
      << L"</h3>\n<p>";

   informationStr
      << L"<table>";   // start table of file formats supported


   #ifdef USE_LIBMAD
   /* i18n-hint: This is what the library (libmad) does - imports MP3 files */
   AddBuildinfoRow(&informationStr, L"libmad", XO("MP3 Importing"), enabled);
   #else
   AddBuildinfoRow(&informationStr, L"libmad", XO("MP3 Importing"), disabled);
   #endif

   #ifdef USE_LIBVORBIS
   AddBuildinfoRow(&informationStr, L"libvorbis",
   /* i18n-hint: Ogg is the container format. Vorbis is the compression codec.
    * Both are proper nouns and shouldn't be translated */
         XO("Ogg Vorbis Import and Export"), enabled);
   #else
   AddBuildinfoRow(&informationStr, L"libvorbis",
         XO("Ogg Vorbis Import and Export"), disabled);
   #endif

   #ifdef USE_LIBID3TAG
   AddBuildinfoRow(&informationStr, L"libid3tag", XO("ID3 tag support"),
         enabled);
   #else
   AddBuildinfoRow(&informationStr, L"libid3tag", XO("ID3 tag support"),
         disabled);
   #endif

   # if USE_LIBFLAC
   /* i18n-hint: FLAC stands for Free Lossless Audio Codec, but is effectively
    * a proper noun and so shouldn't be translated */
   AddBuildinfoRow(&informationStr, L"libflac", XO("FLAC import and export"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"libflac", XO("FLAC import and export"),
         disabled);
   # endif

   # if USE_LIBTWOLAME
   AddBuildinfoRow(&informationStr, L"libtwolame", XO("MP2 export"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"libtwolame", XO("MP2 export"),
         disabled);
   # endif

   # if USE_QUICKTIME
   AddBuildinfoRow(&informationStr, L"QuickTime", XO("Import via QuickTime"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"QuickTime", XO("Import via QuickTime"),
         disabled);
   # endif

   #ifdef USE_FFMPEG
   AddBuildinfoRow(&informationStr, L"ffmpeg", XO("FFmpeg Import/Export"), enabled);
   #else
   AddBuildinfoRow(&informationStr, L"ffmpeg", XO("FFmpeg Import/Export"), disabled);
   #endif

   #ifdef USE_GSTREAMER
   AddBuildinfoRow(&informationStr, L"gstreamer", XO("Import via GStreamer"), enabled);
   #else
   AddBuildinfoRow(&informationStr, L"gstreamer", XO("Import via GStreamer"), disabled);
   #endif

   informationStr << L"</table>\n";  //end table of file formats supported

   informationStr
      << L"<h3>"
      << XO("Features")
      << L"</h3>\n<table>";  // start table of features

#ifdef EXPERIMENTAL_DA
   AddBuildinfoRow(&informationStr, L"Theme", XO("Dark Theme Extras"), enabled);
#else
   AddBuildinfoRow(&informationStr, L"Theme", XO("Dark Theme Extras"), disabled);
#endif

   # if USE_NYQUIST
   AddBuildinfoRow(&informationStr, L"Nyquist", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"Nyquist", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_LADSPA
   AddBuildinfoRow(&informationStr, L"LADSPA", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"LADSPA", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_VAMP
   AddBuildinfoRow(&informationStr, L"Vamp", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"Vamp", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_AUDIO_UNITS
   AddBuildinfoRow(&informationStr, L"Audio Units", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"Audio Units", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_VST
   AddBuildinfoRow(&informationStr, L"VST", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"VST", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_LV2
   AddBuildinfoRow(&informationStr, L"LV2", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"LV2", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_PORTMIXER
   AddBuildinfoRow(&informationStr, L"PortMixer", XO("Sound card mixer support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"PortMixer", XO("Sound card mixer support"),
         disabled);
   # endif

   # if USE_SOUNDTOUCH
   AddBuildinfoRow(&informationStr, L"SoundTouch", XO("Pitch and Tempo Change support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"SoundTouch", XO("Pitch and Tempo Change support"),
         disabled);
   # endif

   # if USE_SBSMS
   AddBuildinfoRow(&informationStr, L"SBSMS", XO("Extreme Pitch and Tempo Change support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"SBSMS", XO("Extreme Pitch and Tempo Change support"),
         disabled);
   # endif

   informationStr << L"</table>\n";   // end of table of features

   html->SetPage( FormatHtmlText( o.GetString() ) );   // push the page into the html renderer
   S.Prop(2)
      .Position( wxEXPAND )
      .AddWindow( html ); // make it fill the page
   // I think the 2 here goes with the StartVerticalLay() call above?
   S.EndVerticalLay();     // end window
   S.EndNotebookPage(); // end the tab
}


const wxString GPL_TEXT();

void AboutDialog::PopulateLicensePage( ShuttleGui & S )
{
#if defined(HAS_PRIVACY_POLICY)
   S.StartNotebookPage(XC("Legal", "about dialog"));
#else
   S.StartNotebookPage(XO("GPL License"));
#endif
   
#if defined(HAS_PRIVACY_POLICY)
   S.Prop(0).StartPanel();
   {
      S.AddSpace(0, 8);
      /* i18n-hint: For "About Audacity...": Title for Privacy Policy section */
      S.AddVariableText(XC("PRIVACY POLICY", "about dialog"), true);

      S.AddFixedText(
         XO("App update checking and error reporting require network access. "
            "These features are optional."));

      /* i18n-hint: %s will be replaced with "our Privacy Policy" */
      AccessibleLinksFormatter privacyPolicy(XO("See %s for more info."));

      privacyPolicy.FormatLink(
         /* i18n-hint: Title of hyperlink to the privacy policy. This is an object of "See". */
         L"%s", XO("our Privacy Policy"),
         "https://www.audacityteam.org/about/desktop-privacy-notice/");

      privacyPolicy.Populate(S);
   }
   S.EndPanel();

   S.AddSpace(0, 8);
#endif

   S.Prop(1).StartPanel();
   {
      HtmlWindow* html = safenew LinkingHtmlWindow(
         S.GetParent(), -1, wxDefaultPosition, wxSize(ABOUT_DIALOG_WIDTH, 264),
         wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);

      html->SetPage(FormatHtmlText(GPL_TEXT()));

      S.Prop(1).Position(wxEXPAND).AddWindow( html );
   }
   S.EndPanel();

   S.EndNotebookPage();
}

void AboutDialog::AddCredit( const wxString &name, Role role )
{
   AddCredit( name, {}, role );
}

void AboutDialog::AddCredit(
   const wxString &name, TranslatableString format, Role role )
{
   auto str = format.empty()
      ? Verbatim( name )
      : TranslatableString{ format }.Format( name );
   creditItems.emplace_back(std::move(str), role);
}

wxString AboutDialog::GetCreditsByRole(AboutDialog::Role role)
{
   wxString s;

   for (const auto &item : creditItems)
   {
      if (item.role == role)
      {
         s += item.description.Translation();
         s += L"<br>";
      }
   }

   // Strip last <br>, if any
   if (s.Right(4) == L"<br>")
      s = s.Left(s.length() - 4);

   return s;
}

/** \brief Add a table row saying if a library is used or not
 *
 * Used when creating the build information tab to show if each optional
 * library is enabled or not, and what it does */
void AboutDialog::AddBuildinfoRow(
   wxTextOutputStream *str, const wxChar * libname,
   const TranslatableString &libdesc, const TranslatableString &status)
{
   *str
      << L"<tr><td>"
      << libname
      << L"</td><td>("
      << libdesc
      << L")</td><td>"
      << status
      << L"</td></tr>";
}

/** \brief Add a table row saying if a library is used or not
 *
 * Used when creating the build information tab to show build dates and
 * file paths */
void AboutDialog::AddBuildinfoRow(
   wxTextOutputStream *str,
   const TranslatableString &description, const wxChar *spec)
{
   *str
      << L"<tr><td>"
      << description
      << L"</td><td>"
      << spec
      << L"</td></tr>";
}

AboutDialog::~AboutDialog()
{
   sActiveInstance = {};
}

void AboutDialog::OnOK(wxCommandEvent & WXUNUSED(event))
{
#ifdef __WXMAC__
   Destroy();
#else
   EndModal(wxID_OK);
#endif
}
