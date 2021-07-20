/**********************************************************************

  Audacity: A Digital Audio Editor

  @file FFmpegPrefs.cpp
  @brief adds controls for FFmpeg import/export to Library preferences

  Paul Licameli split from LibraryPrefs.cpp

**********************************************************************/

#include "FFmpeg.h"
#include "Internat.h"
#include "ShuttleGui.h"
#include "prefs/LibraryPrefs.h"
#include "widgets/AudacityMessageBox.h"
#include "widgets/HelpSystem.h"
#include "widgets/ReadOnlyText.h"
#include <wx/button.h>
#include <wx/stattext.h>

namespace {

struct State {
   wxWindow *parent = nullptr;
   ReadOnlyText *FFmpegVersion = nullptr;
};

void SetFFmpegVersionText(State &state)
{
   auto FFmpegVersion = state.FFmpegVersion;
   FFmpegVersion->SetValue(GetFFmpegVersion());
}

void OnFFmpegFindButton(State &state)
{
#ifdef USE_FFMPEG
   FFmpegLibs* FFmpegLibsPtr = PickFFmpegLibs();
   bool showerrs =
#if defined(_DEBUG)
      true;
#else
      false;
#endif

   FFmpegLibsPtr->FreeLibs();
   // Load the libs ('true' means that all errors will be shown)
   bool locate = !LoadFFmpeg(showerrs);

   // Libs are fine, don't show "locate" dialog unless user really wants it
   if (!locate) {
      int response = AudacityMessageBox(
         XO(
"Audacity has automatically detected valid FFmpeg libraries.\nDo you still want to locate them manually?"),
         XO("Success"),
         wxCENTRE | wxYES_NO | wxNO_DEFAULT |wxICON_QUESTION);
      if (response == wxYES) {
        locate = true;
      }
   }

   if (locate) {
      // Show "Locate FFmpeg" dialog
      FFmpegLibsPtr->FindLibs(state.parent);
      FFmpegLibsPtr->FreeLibs();
      LoadFFmpeg(showerrs);
   }
   SetFFmpegVersionText(state);

   DropFFmpegLibs();
#endif
}

void AddControls( ShuttleGui &S )
{
   auto pState = std::make_shared<State>();
   pState->parent = S.GetParent();

   S.StartStatic(XO("FFmpeg Import/Export Library"));
   {
      S.StartTwoColumn();
      {
         auto version =
 #if defined(USE_FFMPEG)
            XO("No compatible FFmpeg library was found");
 #else
            XO("FFmpeg support is not compiled in");
 #endif

         auto FFmpegVersion = S
           .Position(wxALIGN_CENTRE_VERTICAL)
            .AddReadOnlyText(XO("FFmpeg Library Version:"), version.Translation());

         S.AddVariableText(XO("FFmpeg Library:"),
            true, wxALL | wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL);

         auto pFindButton =
         S
#if !defined(USE_FFMPEG) || defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
            .Disable()
#endif
            .AddButton(XXO("Loca&te..."),
                       wxALL | wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
         pFindButton->Bind(wxEVT_BUTTON, [pState](wxCommandEvent&){
            OnFFmpegFindButton(*pState);
         });

         S.AddVariableText(XO("FFmpeg Library:"),
            true, wxALL | wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL);

         auto pDownButton =
         S
#if !defined(USE_FFMPEG) || defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
            .Disable()
#endif
            .AddButton(XXO("Dow&nload"),
                       wxALL | wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
         pDownButton->Bind(wxEVT_BUTTON, [pState](wxCommandEvent&){
            HelpSystem::ShowHelp(pState->parent,
               wxT("FAQ:Installing_the_FFmpeg_Import_Export_Library"), true);
         });
      }
      S.EndTwoColumn();
   }
   S.EndStatic();

   SetFFmpegVersionText(*pState);
}

LibraryPrefs::RegisteredControls reg{ wxT("FFmpeg"), AddControls };

}
