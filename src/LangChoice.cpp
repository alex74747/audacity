/**********************************************************************

  Audacity: A Digital Audio Editor

  LangChoice.cpp

  Dominic Mazzoni

*******************************************************************//*!

\class LangChoiceDialog
\brief A dialog used (at start up) to present the user with a choice
of languages for Audacity.

*//*******************************************************************/



#include "LangChoice.h"
#include "MemoryX.h"

#include <wx/defs.h>
#include <wx/stattext.h>

#include "Languages.h"
#include "ShuttleGui.h"
#include "widgets/AudacityMessageBox.h"
#include "widgets/wxPanelWrapper.h"

class LangChoiceDialog final : public wxDialogWrapper {
public:
   LangChoiceDialog(wxWindow * parent,
                    wxWindowID id,
                    const TranslatableString & title);

   Identifier GetLang() { return mLangCodes[ mLang ]; }

private:
   void OnOk();

   int mLang = 0;

   int mNumLangs;
   Identifiers mLangCodes;
   TranslatableStrings mLangNames;
};

Identifier ChooseLanguage(wxWindow *parent)
{
   Identifier returnVal;

   /* i18n-hint: Title on a dialog indicating that this is the first
    * time Audacity has been run. */
   LangChoiceDialog dlog(parent, -1, XO("Audacity First Run"));
   dlog.CentreOnParent();
   dlog.ShowModal();
   returnVal = dlog.GetLang();

   return returnVal;
}

LangChoiceDialog::LangChoiceDialog(wxWindow * parent,
                                   wxWindowID id,
                                   const TranslatableString & title):
   wxDialogWrapper(parent, id, title)
{
   using namespace DialogDefinition;

   SetName();
   const auto &paths = FileNames::AudacityPathList();
   Languages::GetLanguages(paths, mLangCodes, mLangNames);
   mLang = make_iterator_range( mLangCodes )
      .index( Languages::GetSystemLanguageCode(paths) );

    ShuttleGui S{ this };

   S.StartVerticalLay(false);
   {
      S.StartHorizontalLay();
      {
         S.SetBorder(15);

         S
            .Target( NumberChoice( mLang, mLangNames ) )
            .AddChoice(XXO("Choose Language for Audacity to use:") );
      }
      S.EndVerticalLay();

      S
         .AddStandardButtons( 0, {
            S.Item( eOkButton ).Action( [this]{ OnOk(); } )
         });
   }
   S.EndVerticalLay();

   Fit();
}

void LangChoiceDialog::OnOk()
{
   auto slang =
      Languages::GetSystemLanguageCode(FileNames::AudacityPathList());
   int sndx = make_iterator_range( mLangCodes ).index( slang );
   wxString sname;

   if (sndx == wxNOT_FOUND) {
      const wxLanguageInfo *sinfo = wxLocale::FindLanguageInfo(slang.GET());
      if (sinfo) {
         sname = sinfo->Description;
      }
   }
   else {
      sname = mLangNames[sndx].Translation();
   }

   auto lang = GetLang();
   if (lang.GET().Left(2) != slang.GET().Left(2)) {
      /* i18n-hint: The %s's are replaced by translated and untranslated
       * versions of language names. */
      auto msg = XO("The language you have chosen, %s (%s), is not the same as the system language, %s (%s).")
         .Format(mLangNames[ mLang ],
                 lang.GET(),
                 sname,
                 slang.GET());
      if ( wxNO == AudacityMessageBox( msg, XO("Confirm"), wxYES_NO ) ) {
         return;
      }
   }

   EndModal(true);
}
