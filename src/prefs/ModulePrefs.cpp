/**********************************************************************

  Audacity: A Digital Audio Editor

  ModulePrefs.cpp

  James Crook

*******************************************************************//**

\class ModulePrefs
\brief A PrefsPanel to enable/disable certain modules.  'Modules' are 
dynamically linked libraries that modify Audacity.  They are plug-ins 
with names like mnod-script-pipe that add NEW features.

*//*******************************************************************/

#include "../Audacity.h"
#include "ModulePrefs.h"



#include <unordered_set>
#include <wx/defs.h>
#include <wx/filename.h>

#include "../ShuttleGui.h"

#include "../Prefs.h"

////////////////////////////////////////////////////////////////////////////////

ModulePrefs::ModulePrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: Modules are optional extensions to Audacity that add NEW features.*/
:  PrefsPanel(parent, winid, XO("Modules"))
{
   Populate();
}

ModulePrefs::~ModulePrefs()
{
}

ComponentInterfaceSymbol ModulePrefs::GetSymbol()
{
   return MODULE_PREFS_PLUGIN_SYMBOL;
}

TranslatableString ModulePrefs::GetDescription()
{
   return XO("Preferences for Module");
}

wxString ModulePrefs::HelpPageName()
{
   return "Modules_Preferences";
}

void ModulePrefs::GetAllModuleStatuses(){
   wxString str;
   long dummy;

   // Modules could for example be:
   //    mod-script-pipe
   //    mod-nyq-bench
   //    mod-menu-munger
   //    mod-theming

   // TODO: On an Audacity upgrade we should (?) actually untick modules.
   // The old modules might be still around, and we do not want to use them.
   mModules.clear();
   mStatuses.clear();
   mPaths.clear();


   // Iterate through all Modules listed in prefs.
   // Get their names and values.
   gPrefs->SetPath( wxT("Module/") );
   bool bCont = gPrefs->GetFirstEntry(str, dummy);
   while ( bCont ) {
      int iStatus;
      gPrefs->Read( str, &iStatus, kModuleDisabled );
      wxString fname;
      gPrefs->Read( wxString( wxT("/ModulePath/") ) + str, &fname, wxEmptyString );
      if( !fname.empty() && wxFileExists( fname ) ){
         if( iStatus > kModuleNew ){
            iStatus = kModuleNew;
            gPrefs->Write( str, iStatus );
         }
         //wxLogDebug( wxT("Entry: %s Value: %i"), str, iStatus );
         mModules.push_back( str );
         mStatuses.push_back( iStatus );
         mPaths.push_back( fname );
      }
      bCont = gPrefs->GetNextEntry(str, dummy);
   }
   gPrefs->SetPath( wxT("") );
}

void ModulePrefs::Populate()
{
   GetAllModuleStatuses();
   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

void ModulePrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic( {} );
   {
      S.AddFixedText(XO(
"These are experimental modules. Enable them only if you've read the Audacity Manual\nand know what you are doing.") );
      S.AddFixedText(XO(
/* i18n-hint preserve the leading spaces */
"  'Ask' means Audacity will ask if you want to load the module each time it starts.") );
      S.AddFixedText(XO(
/* i18n-hint preserve the leading spaces */
"  'Failed' means Audacity thinks the module is broken and won't run it.") );
      S.AddFixedText(XO(
/* i18n-hint preserve the leading spaces */
"  'New' means no choice has been made yet.") );
      S.AddFixedText(XO(
"Changes to these settings only take effect when Audacity starts up."));
      {
        S.StartMultiColumn( 2 );
        int i;
        for(i=0;i<(int)mModules.size();i++)
           S.TieChoice( Verbatim( mModules[i] ),
              mStatuses[i],
              {
                 XO("Disabled" ) ,
                 XO("Enabled" ) ,
                 XO("Ask" ) ,
                 XO("Failed" ) ,
                 XO("New" ) ,
              }
           );
        S.EndMultiColumn();
      }
      if( mModules.size() < 1 )
      {
        S.AddFixedText( XO("No modules were found") );
      }
   }
   S.EndStatic();
   S.EndScroller();
}

bool ModulePrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);
   int i;
   for(i=0;i<(int)mPaths.size();i++)
      SetModuleStatus( mPaths[i], mStatuses[i] );
   return true;
}


static const std::unordered_set<wxString> &autoEnabledModules()
{
   // Add names to this list, of modules that are expected to ship
   // with Audacity and enable automatically.
   static std::unordered_set<wxString> modules{
      "mod-mixer-board",
      "mod-lyrics",
      "mod-undo-history",
      "mod-contrast",
      "mod-frequency-plot",
      "mod-timer-record",
      "mod-clip-menu-items",
      "mod-track-menus",
      "mod-select-menus",
      "mod-view-menus",
      "mod-help-menus",
      "mod-navigation-menus",
      "mod-audiounits",
      "mod-ladspa",
      "mod-lv2",
      "mod-nyquist",
      "mod-vamp",
      "mod-vst",
      "mod-command-classes",
      "mod-screenshot",
      "mod-printing",
      "mod-macros",
      "mod-plugin-menus",
      "mod-flac",
      "mod-ogg",
      "mod-mp2",
      "mod-ffmpeg",
      "mod-mp3",
      "mod-pcm",
      "mod-midi-import-export",
   };
   return modules;
}

// static function that tells us about a module.
int ModulePrefs::GetModuleStatus(const FilePath &fname)
{
   // Default status is NEW module, and we will ask once.
   int iStatus = kModuleNew;

   wxFileName FileName( fname );
   wxString ShortName = FileName.GetName().Lower();

   wxString PathPref = wxString( wxT("/ModulePath/") ) + ShortName;
   wxString StatusPref = wxString( wxT("/Module/") ) + ShortName;
   wxString DateTimePref = wxString( wxT("/ModuleDateTime/") ) + ShortName;

   wxString ModulePath = gPrefs->Read( PathPref, wxEmptyString );
   if( ModulePath.IsSameAs( fname ) )
   {
      gPrefs->Read( StatusPref, &iStatus, kModuleNew );

      wxDateTime DateTime = FileName.GetModificationTime();
      wxDateTime OldDateTime;
      OldDateTime.ParseISOCombined( gPrefs->Read( DateTimePref, wxEmptyString ) );

      // Some platforms return milliseconds, some do not...level the playing field
      DateTime.SetMillisecond( 0 );
      OldDateTime.SetMillisecond( 0 );

      // fix up a bad status or reset for newer module
      if( iStatus > kModuleNew || !OldDateTime.IsEqualTo( DateTime ) )
      {
         iStatus=kModuleNew;
      }
   }
   else
   {
      // Remove previously saved since it's no longer valid
      gPrefs->DeleteEntry( PathPref );
      gPrefs->DeleteEntry( StatusPref );
      gPrefs->DeleteEntry( DateTimePref );
   }

   if (iStatus == kModuleNew) {
      if (autoEnabledModules().count(ShortName))
         iStatus = kModuleEnabled;
   }

   return iStatus;
}

void ModulePrefs::SetModuleStatus(const FilePath &fname, int iStatus)
{
   wxFileName FileName( fname );
   wxDateTime DateTime = FileName.GetModificationTime();
   wxString ShortName = FileName.GetName().Lower();

   wxString PrefName = wxString( wxT("/Module/") ) + ShortName;
   gPrefs->Write( PrefName, iStatus );

   PrefName = wxString( wxT("/ModulePath/") ) + ShortName;
   gPrefs->Write( PrefName, fname );

   PrefName = wxString( wxT("/ModuleDateTime/") ) + ShortName;
   gPrefs->Write( PrefName, DateTime.FormatISOCombined() );

   gPrefs->Flush();
}

#ifdef EXPERIMENTAL_MODULE_PREFS
namespace{
PrefsPanel::Registration sAttachment{ "Module",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew ModulePrefs(parent, winid);
   },
   false,
   // Register with an explicit ordering hint because this one is
   // only conditionally compiled
   { "", { Registry::OrderingHint::After, "Mouse" } }
};
}
#endif
