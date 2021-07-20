/**********************************************************************

  Audacity: A Digital Audio Editor

  @file ModuleSettings.cpp

  Paul Licameli split from ModulePrefs.cpp

**********************************************************************/

#include "ModuleSettings.h"

#include "Prefs.h"

#include <unordered_set>
#include <wx/filename.h>

static const std::unordered_set<wxString> &autoEnabledModules()
{
   // Add names to this list, of modules that are expected to ship
   // with Audacity and enable automatically.
   static std::unordered_set<wxString> modules{
      "mod-soundtouch",
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
      "mod-import-export",
      "mod-tags-ui",
      "mod-tags",
   };
   return modules;
}

// static function that tells us about a module.
int ModuleSettings::GetModuleStatus(const FilePath &fname)
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

void ModuleSettings::SetModuleStatus(const FilePath &fname, int iStatus)
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

