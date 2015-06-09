/**********************************************************************

  Audacity: A Digital Audio Editor

  GUIPrefs.h

  Brian Gunlogson
  Joshua Haberman
  James Crook

**********************************************************************/

#ifndef __AUDACITY_GUI_PREFS__
#define __AUDACITY_GUI_PREFS__

#include <wx/defs.h>

#include <wx/arrstr.h>
#include <wx/window.h>

#include "../Experimental.h"

#include "PrefsPanel.h"

class ShuttleGui;

class GUIPrefs final : public PrefsPanel
{
 public:
   GUIPrefs(wxWindow * parent);
   ~GUIPrefs();
   bool Apply() override;

   static void GetRangeChoices(wxArrayString *pChoices, wxArrayString *pCodes);

 private:
   void Populate();
   void PopulateOrExchange(ShuttleGui & S);

   wxArrayString mLangCodes;
   wxArrayString mLangNames;

   wxArrayString mHtmlHelpCodes;
   wxArrayString mHtmlHelpChoices;

   wxArrayString mRangeCodes;
   wxArrayString mRangeChoices;

#ifdef EXPERIMENTAL_FISHEYE
   wxArrayInt mFisheyeStyleCodes;
   wxArrayString mFisheyeStyleChoices;
#endif
};

class GUIPrefsFactory final : public PrefsPanelFactory
{
public:
   PrefsPanel *Create(wxWindow *parent) override;
};
#endif
