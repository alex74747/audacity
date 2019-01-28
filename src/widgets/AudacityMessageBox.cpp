/**********************************************************************

  Audacity: A Digital Audio Editor

  AudacityMessageBox.cpp

  Paul Licameli split this out of ErrorDialog.cpp

**********************************************************************/

#include "AudacityMessageBox.h"
#include "ErrorDialog.h" // cycle!
#include "Internat.h"

TranslatableString AudacityMessageBoxCaptionStr()
{
   return XO("Message");
}

int AudacityMessageBox(const TranslatableString& message,
   const TranslatableString& caption,
   long style,
   wxWindow *parent,
   int x, int y)
{
//   return ::wxMessageBox(message, caption, style, parent, x, y);
// PRL:  cut and pasted the definition of wxMessageBox from wxWidgets 3.1.1,
// substituting our own AudacityMessageDialog class

    // add the appropriate icon unless this was explicitly disabled by use of
    // wxICON_NONE
    if ( !(style & wxICON_NONE) && !(style & wxICON_MASK) )
    {
        style |= style & wxYES ? wxICON_QUESTION : wxICON_INFORMATION;
    }

    AudacityMessageDialog dialog(parent, message, caption, style);

    int ans = dialog.ShowModal();
    switch ( ans )
    {
        case wxID_OK:
            return wxOK;
        case wxID_YES:
            return wxYES;
        case wxID_NO:
            return wxNO;
        case wxID_CANCEL:
            return wxCANCEL;
        case wxID_HELP:
            return wxHELP;
    }

    wxFAIL_MSG( wxT("unexpected return code from wxMessageDialog") );

    return wxCANCEL;
}
