/**********************************************************************

  Audacity: A Digital Audio Editor

  Prefs.h

  Dominic Mazzoni
  Markus Meyer

  Audacity uses wxWidgets' wxFileConfig class to handle preferences.
  In Audacity versions prior to 1.3.1, it used wxConfig, which would
  store the prefs in a platform-dependent way (e.g. in the registry
  on Windows). Now it always stores the settings in a configuration file
  in the Audacity Data Directory.

  Every time we read a preference, we need to specify the default
  value for that preference, to be used if the preference hasn't
  been set before.

  So, to avoid code duplication, we provide functions in this file
  to read and write preferences which have a nonobvious default
  value, so that if we later want to change this value, we only
  have to change it in one place.

  See Prefs.cpp for a (complete?) list of preferences we keep
  track of...

**********************************************************************/
#ifndef __AUDACITY_PREFS__
#define __AUDACITY_PREFS__

#include "Audacity.h"

#include <wx/config.h>
#include <wx/fileconf.h>

void InitPreferences();
void FinishPreferences();

extern AUDACITY_DLL_API wxFileConfig *gPrefs;
extern int gMenusDirty;

#ifdef __WXMAC__
// For bug1567, override the flushing behavior of wxFileConfig so that the
// inode number of the destination file does not change.
class MyWxFileConfig : public wxFileConfig
{
public:
   MyWxFileConfig(const wxString& appName = wxEmptyString,
                  const wxString& vendorName = wxEmptyString,
                  const wxString& localFilename = wxEmptyString,
                  const wxString& globalFilename = wxEmptyString,
                  long style = wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_GLOBAL_FILE,
                  const wxMBConv& conv = wxConvAuto());
   ~MyWxFileConfig();

   bool Flush(bool bCurrentOnly = false) override;

private:
   static wxString GetTempFileName(const wxString &localName);
   static wxString CreateTempFile(const wxString &localName);
   wxString mLocalFilename;
};
#else
using MyWxFileConfig = wxFileConfig;
#endif

#endif
