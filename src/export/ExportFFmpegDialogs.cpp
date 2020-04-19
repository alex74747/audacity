/**********************************************************************

   Audacity: A Digital Audio Editor

   ExportFFmpegDialogs.cpp

   Audacity(R) is copyright (c) 1999-2010 Audacity Team.
   License: GPL v2 or later.  See License.txt.

   LRN

******************************************************************//**

\class ExportFFmpegAC3Options
\brief Options dialog for FFmpeg exporting of AC3 format.

*//***************************************************************//**

\class ExportFFmpegAACOptions
\brief Options dialog for FFmpeg exporting of AAC format.

*//***************************************************************//**

\class ExportFFmpegAMRNBOptions
\brief Options dialog for FFmpeg exporting of AMRNB format.

*//***************************************************************//**

\class ExportFFmpegOPUSOptions
\brief Options dialog for FFmpeg exporting of OPUS format.

*//***************************************************************//**

\class ExportFFmpegWMAOptions
\brief Options dialog for FFmpeg exporting of WMA format.

*//***************************************************************//**

\class ExportFFmpegOptions
\brief Options dialog for Custom FFmpeg export format.

*//*******************************************************************/


#include "ExportFFmpegDialogs.h"

#include "../FFmpeg.h"
#include "FFmpegFunctions.h"

#include <wx/app.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/window.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/stattext.h>

#include "../widgets/FileDialog/FileDialog.h"

#include "Mix.h"
#include "../Tags.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/HelpSystem.h"

#include "Export.h"
#include "FFmpeg.h"

#if defined(USE_FFMPEG)

/// This construction defines a enumeration of UI element IDs, and a static
/// array of their string representations (this way they're always synchronized).
/// Do not store the enumerated values in external files, as they may change;
/// the strings may be stored.
#define FFMPEG_EXPORT_CTRL_ID_ENTRIES \
   FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY(FEFirstID, 20000), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEFormatID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FECodecID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEBitrateID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEQualityID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FESampleRateID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FELanguageID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FETagID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FECutoffID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEFrameSizeID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEBufSizeID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEProfileID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FECompLevelID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEUseLPCID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FELPCCoeffsID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMinPredID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMaxPredID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEPredOrderID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMinPartOrderID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMaxPartOrderID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMuxRateID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEPacketSizeID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEBitReservoirID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEVariableBlockLenID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FELastID), \
 \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEPresetID), \

// First the enumeration
#define FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY(name, num)  name = num
#define FFMPEG_EXPORT_CTRL_ID_ENTRY(name)             name

enum FFmpegExportCtrlID {
   FFMPEG_EXPORT_CTRL_ID_ENTRIES
};

// Now the string representations
#undef FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY
#define FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY(name, num)  (L"" #name)
#undef FFMPEG_EXPORT_CTRL_ID_ENTRY
#define FFMPEG_EXPORT_CTRL_ID_ENTRY(name)             (L"" #name)
static const wxChar *FFmpegExportCtrlIDNames[] = {
   FFMPEG_EXPORT_CTRL_ID_ENTRIES
};

#undef FFMPEG_EXPORT_CTRL_ID_ENTRIES
#undef FFMPEG_EXPORT_CTRL_ID_ENTRY
#undef FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY

//----------------------------------------------------------------------------
// ExportFFmpegAC3Options Class
//----------------------------------------------------------------------------

namespace
{

// i18n-hint kbps abbreviates "thousands of bits per second"
inline TranslatableString n_kbps(int n) { return XO("%d kbps").Format( n ); }

const TranslatableStrings AC3BitRateNames{
   n_kbps( 32 ),
   n_kbps( 40 ),
   n_kbps( 48 ),
   n_kbps( 56 ),
   n_kbps( 64 ),
   n_kbps( 80 ),
   n_kbps( 96 ),
   n_kbps( 112 ),
   n_kbps( 128 ),
   n_kbps( 160 ),
   n_kbps( 192 ),
   n_kbps( 224 ),
   n_kbps( 256 ),
   n_kbps( 320 ),
   n_kbps( 384 ),
   n_kbps( 448 ),
   n_kbps( 512 ),
   n_kbps( 576 ),
   n_kbps( 640 ),
};

const std::vector< int > AC3BitRateValues{
   32000,
   40000,
   48000,
   56000,
   64000,
   80000,
   96000,
   112000,
   128000,
   160000,
   192000,
   224000,
   256000,
   320000,
   384000,
   448000,
   512000,
   576000,
   640000,
};

}

const int ExportFFmpegAC3Options::iAC3SampleRates[] = { 32000, 44100, 48000, 0 };

ExportFFmpegAC3Options::ExportFFmpegAC3Options(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegAC3Options::~ExportFFmpegAC3Options()
{
   TransferDataFromWindow();
}

IntSetting AC3BitRate{ L"/FileFormats/AC3BitRate", 160000 };

///
///
void ExportFFmpegAC3Options::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S
               .Target( NumberChoice(
                  AC3BitRate, AC3BitRateNames, AC3BitRateValues ) )
               .AddChoice( XXO("Bit Rate:") );
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegAC3Options::TransferDataFromWindow()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegAACOptions Class
//----------------------------------------------------------------------------

ExportFFmpegAACOptions::ExportFFmpegAACOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegAACOptions::~ExportFFmpegAACOptions()
{
   TransferDataFromWindow();
}

IntSetting AACQuality{ L"/FileFormats/AACQuality", 100 };

///
///
void ExportFFmpegAACOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxEXPAND);
      {
         S.SetSizerProportion(1);
         S.StartMultiColumn(2, wxCENTER);
         {
            S.SetStretchyCol(1);

            S.Prop(1)
               .Target( AACQuality )
               .AddSlider(XXO("Quality (kbps):"), 0, 320, 98);
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegAACOptions::TransferDataFromWindow()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegAMRNBOptions Class
//----------------------------------------------------------------------------

namespace {

// i18n-hint kbps abbreviates "thousands of bits per second"
inline TranslatableString f_kbps( double d ) { return XO("%.2f kbps").Format( d ); }

/// Bit Rates supported by libAMR-NB encoder
/// Sample Rate is always 8 kHz
const TranslatableStrings AMRNBBitRateNames
{
   f_kbps( 4.75 ),
   f_kbps( 5.15 ),
   f_kbps( 5.90 ),
   f_kbps( 6.70 ),
   f_kbps( 7.40 ),
   f_kbps( 7.95 ),
   f_kbps( 10.20 ),
   f_kbps( 12.20 ),
};

const std::vector< int > AMRNBBitRateValues
{
   4750,
   5150,
   5900,
   6700,
   7400,
   7950,
   10200,
   12200,
};

}

ExportFFmpegAMRNBOptions::ExportFFmpegAMRNBOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegAMRNBOptions::~ExportFFmpegAMRNBOptions()
{
   TransferDataFromWindow();
}

IntSetting AMRNBBitRate{ L"/FileFormats/AMRNBBitRate", 12200 };

///
///
void ExportFFmpegAMRNBOptions::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S
               .Target( NumberChoice(
                  AMRNBBitRate, AMRNBBitRateNames, AMRNBBitRateValues ) )
               .AddChoice( XXO("Bit Rate:") );
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegAMRNBOptions::TransferDataFromWindow()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegOPUSOptions Class
//----------------------------------------------------------------------------

namespace {

   /// Bit Rates supported by OPUS encoder. Setting bit rate to other values will not result in different file size.
   ChoiceSetting OPUSBitrate
   {
      L"/FileFormats/OPUSBitrate",
      {
         ByColumns,
         {
            n_kbps( 6 ),
            n_kbps( 8 ),
            n_kbps( 16 ),
            n_kbps( 24 ),
            n_kbps( 32 ),
            n_kbps( 40 ),
            n_kbps( 48 ),
            n_kbps( 64 ),
            n_kbps( 80 ),
            n_kbps( 96 ),
            n_kbps( 128 ),
            n_kbps( 160 ),
            n_kbps( 192 ),
            n_kbps( 256 ),
         },
         {
            L"6000",
            L"8000",
            L"16000",
            L"24000",
            L"32000",
            L"40000",
            L"48000",
            L"64000",
            L"80000",
            L"96000",
            L"128000",
            L"160000",
            L"192000",
            L"256000",
         }
      },
      7 // "128 kbps"
   };

   ChoiceSetting OPUSCompression
   {
      L"/FileFormats/OPUSCompression",
      {
         ByColumns,
         {
            XO("0"),
            XO("1"),
            XO("2"),
            XO("3"),
            XO("4"),
            XO("5"),
            XO("6"),
            XO("7"),
            XO("8"),
            XO("9"),
            XO("10"),
         },
         {
            L"0",
            L"1",
            L"2",
            L"3",
            L"4",
            L"5",
            L"6",
            L"7",
            L"8",
            L"9",
            L"10",
         }
      },
      10 // "10"
   };


   ChoiceSetting OPUSVbrMode
   {
      L"/FileFormats/OPUSVbrMode",
      {
         ByColumns,
         {
            XO("Off"),
            XO("On"),
            XO("Constrained"),
         },
         {
            L"off",
            L"on",
            L"constrained",
         }
      },
      1 // "On"
   };

   ChoiceSetting OPUSApplication
   {
      L"/FileFormats/OPUSApplication",
      {
         ByColumns,
         {
            XO("VOIP"),
            XO("Audio"),
            XO("Low Delay"),
         },
         {
            L"voip",
            L"audio",
            L"lowdelay",
         }
      },
      1 // "Audio"
   };

   ChoiceSetting OPUSFrameDuration
   {
      L"/FileFormats/OPUSFrameDuration",
      {
         ByColumns,
         {
            XO("2.5 ms"),
            XO("5 ms"),
            XO("10 ms"),
            XO("20 ms"),
            XO("40 ms"),
            XO("60 ms"),
         },
         {
            L"2.5",
            L"5",
            L"10",
            L"20",
            L"40",
            L"60",
         }
      },
      3 // "20"
   };

   ChoiceSetting OPUSCutoff
   {
      L"/FileFormats/OPUSCutoff",
      {
         ByColumns,
         {
            XO("Disabled"),
            XO("Narrowband"),
            XO("Mediumband"),
            XO("Wideband"),
            XO("Super Wideband"),
            XO("Fullband"),
         },
         {
            L"0",
            L"4000",
            L"6000",
            L"8000",
            L"12000",
            L"20000",
         }
      },
      0 // "Disabled"
   };
}

ExportFFmpegOPUSOptions::ExportFFmpegOPUSOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegOPUSOptions::~ExportFFmpegOPUSOptions()
{
   TransferDataFromWindow();
}

///
///
void ExportFFmpegOPUSOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.SetSizerProportion(1);
   S.SetBorder(4);
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S.StartMultiColumn(2, wxCENTER);
            {
               S
                  .Target( OPUSBitrate )
                  .AddChoice( XXO("Bit Rate:") );

               S
                  .Target( OPUSCompression )
                  .AddChoice( XXO("Compression") );

               S
                  .Target( OPUSFrameDuration )
                  .AddChoice( XXO("Frame Duration:") );
            }
            S.EndMultiColumn();

            S.StartMultiColumn(2, wxCENTER);
            {
               S
                  .Target( OPUSVbrMode )
                  .AddChoice( XXO("Vbr Mode:") );

               S
                  .Target( OPUSApplication )
                  .AddChoice( XXO("Application:") );

               S
                  .Target( OPUSCutoff )
                  .AddChoice( XXO("Cutoff:") );
            }
            S.EndMultiColumn();
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegOPUSOptions::TransferDataToWindow()
{
   return true;
}

///
///
bool ExportFFmpegOPUSOptions::TransferDataFromWindow()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegWMAOptions Class
//----------------------------------------------------------------------------

const int ExportFFmpegWMAOptions::iWMASampleRates[] =
{ 8000, 11025, 16000, 22050, 44100, 0};

namespace {

/// Bit Rates supported by WMA encoder. Setting bit rate to other values will not result in different file size.
const TranslatableStrings WMABitRateNames
{
   n_kbps(24),
   n_kbps(32),
   n_kbps(40),
   n_kbps(48),
   n_kbps(64),
   n_kbps(80),
   n_kbps(96),
   n_kbps(128),
   n_kbps(160),
   n_kbps(192),
   n_kbps(256),
   n_kbps(320),
};

const std::vector< int > WMABitRateValues{
   24000,
   32000,
   40000,
   48000,
   64000,
   80000,
   96000,
   128000,
   160000,
   192000,
   256000,
   320000,
};

}

ExportFFmpegWMAOptions::ExportFFmpegWMAOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegWMAOptions::~ExportFFmpegWMAOptions()
{
   TransferDataFromWindow();
}

IntSetting WMABitRate{ L"/FileFormats/WMABitRate", 128000 };

///
///
void ExportFFmpegWMAOptions::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S
               .Target( NumberChoice(
                  WMABitRate, WMABitRateNames, WMABitRateValues ) )
               .AddChoice( XXO("Bit Rate:") );
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegWMAOptions::TransferDataFromWindow()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegCustomOptions Class
//----------------------------------------------------------------------------

ExportFFmpegCustomOptions::ExportFFmpegCustomOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY),
   mFormat(NULL),
   mCodec(NULL)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegCustomOptions::~ExportFFmpegCustomOptions()
{
   TransferDataFromWindow();
}

///
///
void ExportFFmpegCustomOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.StartHorizontalLay(wxCENTER);
   {
      S.StartVerticalLay(wxCENTER, 0);
      {
         S
            .Action( [this]{ OnOpen(); } )
            .AddButton(XXO("Open custom FFmpeg format options"));
         S.StartMultiColumn(2, wxCENTER);
         {
            S
               .AddPrompt(XXO("Current Format:"));

            mFormat =
            S
               .Style(wxTE_READONLY)
               .AddTextBox({}, L"", 25);

            S
               .AddPrompt(XXO("Current Codec:"));

            mCodec =
            S
               .Style(wxTE_READONLY)
               .AddTextBox({}, L"", 25);
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndHorizontalLay();
}

///
///
bool ExportFFmpegCustomOptions::TransferDataToWindow()
{
   if (mFormat)
   {
      mFormat->SetValue( FFmpegFormat.Read() );
      mCodec->SetValue( FFmpegCodec.Read() );
   }
   return wxPanelWrapper::TransferDataToWindow();
}

///
///

bool ExportFFmpegCustomOptions::TransferDataFromWindow()
{
   return true;
}

///
///
void ExportFFmpegCustomOptions::OnOpen()
{
   // Show "Locate FFmpeg" dialog
   auto ffmpeg = FFmpegFunctions::Load();
   if (!ffmpeg)
   {
      FindFFmpegLibs();
      if (!LoadFFmpeg(true))
      {
         return;
      }
   }

#ifdef __WXMAC__
   // Bug 2077 Must be a parent window on OSX or we will appear behind.
   auto pWin = wxGetTopLevelParent( this );
#else
   // Use GetTopWindow on windows as there is no hWnd with top level parent.
   auto pWin = wxTheApp->GetTopWindow();
#endif

   ExportFFmpegOptions od(pWin);
   od.ShowModal();

   TransferDataToWindow();
}

FFmpegPreset::FFmpegPreset()
{
   mControlState.resize(FELastID - FEFirstID);
}

FFmpegPreset::~FFmpegPreset()
{
}

FFmpegPresets::FFmpegPresets()
{
   mPreset = NULL;
   mAbortImport = false;

   XMLFileReader xmlfile;
   wxFileName xmlFileName(FileNames::DataDir(), L"ffmpeg_presets.xml");
   xmlfile.Parse(this,xmlFileName.GetFullPath());
}

FFmpegPresets::~FFmpegPresets()
{
   // We're in a destructor!  Don't let exceptions out!
   GuardedCall( [&] {
      wxFileName xmlFileName{ FileNames::DataDir(), L"ffmpeg_presets.xml" };
      XMLFileWriter writer{
         xmlFileName.GetFullPath(), XO("Error Saving FFmpeg Presets") };
      WriteXMLHeader(writer);
      WriteXML(writer);
      writer.Commit();
   } );
}

void FFmpegPresets::ImportPresets(wxString &filename)
{
   mPreset = NULL;
   mAbortImport = false;

   FFmpegPresetMap savePresets = mPresets;

   XMLFileReader xmlfile;
   bool success = xmlfile.Parse(this,filename);
   if (!success || mAbortImport) {
      mPresets = savePresets;
   }
}

void FFmpegPresets::ExportPresets(wxString &filename)
{
   GuardedCall( [&] {
      XMLFileWriter writer{ filename, XO("Error Saving FFmpeg Presets") };
      WriteXMLHeader(writer);
      WriteXML(writer);
      writer.Commit();
   } );
}

void FFmpegPresets::GetPresetList(wxArrayString &list)
{
   list.clear();
   FFmpegPresetMap::iterator iter;
   for (iter = mPresets.begin(); iter != mPresets.end(); ++iter)
   {
      list.push_back(iter->second.mPresetName);
   }

   std::sort( list.begin(), list.end() );
}

void FFmpegPresets::DeletePreset(wxString &name)
{
   FFmpegPresetMap::iterator iter = mPresets.find(name);
   if (iter != mPresets.end())
   {
      mPresets.erase(iter);
   }
}

FFmpegPreset *FFmpegPresets::FindPreset(wxString &name)
{
   FFmpegPresetMap::iterator iter = mPresets.find(name);
   if (iter != mPresets.end())
   {
      return &iter->second;
   }

   return NULL;
}

// return false if overwrite was not allowed.
bool FFmpegPresets::OverwriteIsOk( wxString &name )
{
   FFmpegPreset *preset = FindPreset(name);
   if (preset)
   {
      auto query = XO("Overwrite preset '%s'?").Format(name);
      int action = AudacityMessageBox(
         query,
         XO("Confirm Overwrite"),
         wxYES_NO | wxCENTRE);
      if (action == wxNO) return false;
   }
   return true;
}


bool FFmpegPresets::SavePreset(ExportFFmpegOptions *parent, wxString &name)
{
   wxString format;
   wxString codec;
   FFmpegPreset *preset;

   {
      wxWindow *wnd;
      wxListBox *lb;

      wnd = dynamic_cast<wxWindow*>(parent)->FindWindowById(FEFormatID,parent);
      lb = dynamic_cast<wxListBox*>(wnd);
      if (lb->GetSelection() < 0)
      {
         AudacityMessageBox( XO("Please select format before saving a profile") );
         return false;
      }
      format = lb->GetStringSelection();

      wnd = dynamic_cast<wxWindow*>(parent)->FindWindowById(FECodecID,parent);
      lb = dynamic_cast<wxListBox*>(wnd);
      if (lb->GetSelection() < 0)
      {
         /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
         AudacityMessageBox( XO("Please select codec before saving a profile") );
         return false;
      }
      codec = lb->GetStringSelection();
   }

   preset = &mPresets[name];
   preset->mPresetName = name;

   wxSpinCtrl *sc;
   wxTextCtrl *tc;
   wxCheckBox *cb;
   wxChoice *ch;

   for (int id = FEFirstID; id < FELastID; id++)
   {
      wxWindow *wnd = dynamic_cast<wxWindow*>(parent)->FindWindowById(id,parent);
      if (wnd != NULL)
      {
         switch(id)
         {
         case FEFormatID:
            preset->mControlState[id - FEFirstID] = format;
            break;
         case FECodecID:
            preset->mControlState[id - FEFirstID] = codec;
            break;
         // Spin control
         case FEBitrateID:
         case FEQualityID:
         case FESampleRateID:
         case FECutoffID:
         case FEFrameSizeID:
         case FEBufSizeID:
         case FECompLevelID:
         case FELPCCoeffsID:
         case FEMinPredID:
         case FEMaxPredID:
         case FEMinPartOrderID:
         case FEMaxPartOrderID:
         case FEMuxRateID:
         case FEPacketSizeID:
            sc = dynamic_cast<wxSpinCtrl*>(wnd);
            preset->mControlState[id - FEFirstID] = wxString::Format(L"%d",sc->GetValue());
            break;
         // Text control
         case FELanguageID:
         case FETagID:
            tc = dynamic_cast<wxTextCtrl*>(wnd);
            preset->mControlState[id - FEFirstID] = tc->GetValue();
            break;
         // Choice
         case FEProfileID:
         case FEPredOrderID:
            ch = dynamic_cast<wxChoice*>(wnd);
            preset->mControlState[id - FEFirstID] = wxString::Format(L"%d",ch->GetSelection());
            break;
         // Check box
         case FEUseLPCID:
         case FEBitReservoirID:
         case FEVariableBlockLenID:
            cb = dynamic_cast<wxCheckBox*>(wnd);
            preset->mControlState[id - FEFirstID] = wxString::Format(L"%d",cb->GetValue());
            break;
         }
      }
   }
   return true;
}

void FFmpegPresets::LoadPreset(ExportFFmpegOptions *parent, wxString &name)
{
   FFmpegPreset *preset = FindPreset(name);
   if (!preset)
   {
      AudacityMessageBox( XO("Preset '%s' does not exist." ).Format(name));
      return;
   }

   wxListBox *lb;
   wxSpinCtrl *sc;
   wxTextCtrl *tc;
   wxCheckBox *cb;
   wxChoice *ch;

   for (int id = FEFirstID; id < FELastID; id++)
   {
      wxWindow *wnd = parent->FindWindowById(id,parent);
      if (wnd != NULL)
      {
         wxString readstr;
         long readlong;
         bool readbool;
         switch(id)
         {
         // Listbox
         case FEFormatID:
         case FECodecID:
            lb = dynamic_cast<wxListBox*>(wnd);
            readstr = preset->mControlState[id - FEFirstID];
            readlong = lb->FindString(readstr);
            if (readlong > -1) lb->Select(readlong);
            break;
         // Spin control
         case FEBitrateID:
         case FEQualityID:
         case FESampleRateID:
         case FECutoffID:
         case FEFrameSizeID:
         case FEBufSizeID:
         case FECompLevelID:
         case FELPCCoeffsID:
         case FEMinPredID:
         case FEMaxPredID:
         case FEMinPartOrderID:
         case FEMaxPartOrderID:
         case FEMuxRateID:
         case FEPacketSizeID:
            sc = dynamic_cast<wxSpinCtrl*>(wnd);
            preset->mControlState[id - FEFirstID].ToLong(&readlong);
            sc->SetValue(readlong);
            break;
         // Text control
         case FELanguageID:
         case FETagID:
            tc = dynamic_cast<wxTextCtrl*>(wnd);
            tc->SetValue(preset->mControlState[id - FEFirstID]);
            break;
         // Choice
         case FEProfileID:
         case FEPredOrderID:
            ch = dynamic_cast<wxChoice*>(wnd);
            preset->mControlState[id - FEFirstID].ToLong(&readlong);
            if (readlong > -1) ch->Select(readlong);
            break;
         // Check box
         case FEUseLPCID:
         case FEBitReservoirID:
         case FEVariableBlockLenID:
            cb = dynamic_cast<wxCheckBox*>(wnd);
            preset->mControlState[id - FEFirstID].ToLong(&readlong);
            if (readlong) readbool = true; else readbool = false;
            cb->SetValue(readbool);
            break;
         }
      }
   }
}

bool FFmpegPresets::HandleXMLTag(const std::string_view& tag, const AttributesList &attrs)
{
   if (mAbortImport)
   {
      return false;
   }

   if (tag == "ffmpeg_presets")
   {
      return true;
   }

   if (tag == "preset")
   {
      for (auto pair : attrs)
      {
         auto attr = pair.first;
         auto value = pair.second;

         if (attr == "name")
         {
            wxString strValue = value.ToWString();
            mPreset = FindPreset(strValue);

            if (mPreset)
            {
               auto query = XO("Replace preset '%s'?").Format( strValue );
               int action = AudacityMessageBox(
                  query,
                  XO("Confirm Overwrite"),
                  wxYES_NO | wxCANCEL | wxCENTRE);
               if (action == wxCANCEL)
               {
                  mAbortImport = true;
                  return false;
               }
               if (action == wxNO)
               {
                  mPreset = NULL;
                  return false;
               }
               *mPreset = FFmpegPreset();
            }
            else
            {
               mPreset = &mPresets[strValue];
            }

            mPreset->mPresetName = strValue;
         }
      }
      return true;
   }

   if (tag == "setctrlstate" && mPreset)
   {
      long id = -1;
      for (auto pair : attrs)
      {
         auto attr = pair.first;
         auto value = pair.second;

         if (attr == "id")
         {
            for (long i = FEFirstID; i < FELastID; i++)
               if (!wxStrcmp(FFmpegExportCtrlIDNames[i - FEFirstID], value.ToWString()))
                  id = i;
         }
         else if (attr == "state")
         {
            if (id > FEFirstID && id < FELastID)
               mPreset->mControlState[id - FEFirstID] = value.ToWString();
         }
      }
      return true;
   }

   return false;
}

XMLTagHandler *FFmpegPresets::HandleXMLChild(const std::string_view& tag)
{
   if (mAbortImport)
   {
      return NULL;
   }

   if (tag == "preset")
   {
      return this;
   }
   else if (tag == "setctrlstate")
   {
      return this;
   }
   return NULL;
}

void FFmpegPresets::WriteXMLHeader(XMLWriter &xmlFile) const
// may throw
{
   xmlFile.Write(L"<?xml ");
   xmlFile.Write(L"version=\"1.0\" ");
   xmlFile.Write(L"standalone=\"no\" ");
   xmlFile.Write(L"?>\n");

   wxString dtdName = L"-//audacityffmpegpreset-1.0.0//DTD//EN";
   wxString dtdURI =
      L"http://audacity.sourceforge.net/xml/audacityffmpegpreset-1.0.0.dtd";

   xmlFile.Write(L"<!DOCTYPE ");
   xmlFile.Write(L"project ");
   xmlFile.Write(L"PUBLIC ");
   xmlFile.Write(L"\"-//audacityffmpegpreset-1.0.0//DTD//EN\" ");
   xmlFile.Write(L"\"http://audacity.sourceforge.net/xml/audacityffmpegpreset-1.0.0.dtd\" ");
   xmlFile.Write(L">\n");
}

void FFmpegPresets::WriteXML(XMLWriter &xmlFile) const
// may throw
{
   xmlFile.StartTag(L"ffmpeg_presets");
   xmlFile.WriteAttr(L"version",L"1.0");
   FFmpegPresetMap::const_iterator iter;
   for (iter = mPresets.begin(); iter != mPresets.end(); ++iter)
   {
      auto preset = &iter->second;
      xmlFile.StartTag(L"preset");
      xmlFile.WriteAttr(L"name",preset->mPresetName);
      for (long i = FEFirstID + 1; i < FELastID; i++)
      {
         xmlFile.StartTag(L"setctrlstate");
         xmlFile.WriteAttr(L"id",wxString(FFmpegExportCtrlIDNames[i - FEFirstID]));
         xmlFile.WriteAttr(L"state",preset->mControlState[i - FEFirstID]);
         xmlFile.EndTag(L"setctrlstate");
      }
      xmlFile.EndTag(L"preset");
   }
   xmlFile.EndTag(L"ffmpeg_presets");
}

//----------------------------------------------------------------------------
// ExportFFmpegOptions Class
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ExportFFmpegOptions, wxDialogWrapper)
   EVT_LISTBOX(FEFormatID,ExportFFmpegOptions::OnFormatList)
   EVT_LISTBOX(FECodecID,ExportFFmpegOptions::OnCodecList)
END_EVENT_TABLE()

/// Format-codec compatibility list
/// Must end with NULL entry
CompatibilityEntry ExportFFmpegOptions::CompatibilityList[] =
{
   { L"adts", AUDACITY_AV_CODEC_ID_AAC },

   { L"aiff", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   { L"aiff", AUDACITY_AV_CODEC_ID_PCM_S8 },
   { L"aiff", AUDACITY_AV_CODEC_ID_PCM_S24BE },
   { L"aiff", AUDACITY_AV_CODEC_ID_PCM_S32BE },
   { L"aiff", AUDACITY_AV_CODEC_ID_PCM_ALAW },
   { L"aiff", AUDACITY_AV_CODEC_ID_PCM_MULAW },
   { L"aiff", AUDACITY_AV_CODEC_ID_MACE3 },
   { L"aiff", AUDACITY_AV_CODEC_ID_MACE6 },
   { L"aiff", AUDACITY_AV_CODEC_ID_GSM },
   { L"aiff", AUDACITY_AV_CODEC_ID_ADPCM_G726 },
   { L"aiff", AUDACITY_AV_CODEC_ID_PCM_S16LE },
   { L"aiff", AUDACITY_AV_CODEC_ID_ADPCM_IMA_QT },
   { L"aiff", AUDACITY_AV_CODEC_ID_QDM2 },

   { L"amr", AUDACITY_AV_CODEC_ID_AMR_NB },
   { L"amr", AUDACITY_AV_CODEC_ID_AMR_WB },

   { L"asf", AUDACITY_AV_CODEC_ID_PCM_S16LE },
   { L"asf", AUDACITY_AV_CODEC_ID_PCM_U8 },
   { L"asf", AUDACITY_AV_CODEC_ID_PCM_S24LE },
   { L"asf", AUDACITY_AV_CODEC_ID_PCM_S32LE },
   { L"asf", AUDACITY_AV_CODEC_ID_ADPCM_MS },
   { L"asf", AUDACITY_AV_CODEC_ID_PCM_ALAW },
   { L"asf", AUDACITY_AV_CODEC_ID_PCM_MULAW },
   { L"asf", AUDACITY_AV_CODEC_ID_WMAVOICE },
   { L"asf", AUDACITY_AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"asf", AUDACITY_AV_CODEC_ID_ADPCM_YAMAHA },
   { L"asf", AUDACITY_AV_CODEC_ID_TRUESPEECH },
   { L"asf", AUDACITY_AV_CODEC_ID_GSM_MS },
   { L"asf", AUDACITY_AV_CODEC_ID_ADPCM_G726 },
   //{ L"asf", AUDACITY_AV_CODEC_ID_MP2 }, Bug 59
   { L"asf", AUDACITY_AV_CODEC_ID_MP3 },
#if LIBAVCODEC_VERSION_MAJOR < 58
   { L"asf", AUDACITY_AV_CODEC_ID_VOXWARE },
#endif
   { L"asf", AUDACITY_AV_CODEC_ID_AAC },
   { L"asf", AUDACITY_AV_CODEC_ID_WMAV1 },
   { L"asf", AUDACITY_AV_CODEC_ID_WMAV2 },
   { L"asf", AUDACITY_AV_CODEC_ID_WMAPRO },
   { L"asf", AUDACITY_AV_CODEC_ID_ADPCM_CT },
   { L"asf", AUDACITY_AV_CODEC_ID_ATRAC3 },
   { L"asf", AUDACITY_AV_CODEC_ID_IMC },
   { L"asf", AUDACITY_AV_CODEC_ID_AC3 },
   { L"asf", AUDACITY_AV_CODEC_ID_DTS },
   { L"asf", AUDACITY_AV_CODEC_ID_FLAC },
   { L"asf", AUDACITY_AV_CODEC_ID_ADPCM_SWF },
   { L"asf", AUDACITY_AV_CODEC_ID_VORBIS },

   { L"au", AUDACITY_AV_CODEC_ID_PCM_MULAW },
   { L"au", AUDACITY_AV_CODEC_ID_PCM_S8 },
   { L"au", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   { L"au", AUDACITY_AV_CODEC_ID_PCM_ALAW },

   { L"avi", AUDACITY_AV_CODEC_ID_PCM_S16LE },
   { L"avi", AUDACITY_AV_CODEC_ID_PCM_U8 },
   { L"avi", AUDACITY_AV_CODEC_ID_PCM_S24LE },
   { L"avi", AUDACITY_AV_CODEC_ID_PCM_S32LE },
   { L"avi", AUDACITY_AV_CODEC_ID_ADPCM_MS },
   { L"avi", AUDACITY_AV_CODEC_ID_PCM_ALAW },
   { L"avi", AUDACITY_AV_CODEC_ID_PCM_MULAW },
   { L"avi", AUDACITY_AV_CODEC_ID_WMAVOICE },
   { L"avi", AUDACITY_AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"avi", AUDACITY_AV_CODEC_ID_ADPCM_YAMAHA },
   { L"avi", AUDACITY_AV_CODEC_ID_TRUESPEECH },
   { L"avi", AUDACITY_AV_CODEC_ID_GSM_MS },
   { L"avi", AUDACITY_AV_CODEC_ID_ADPCM_G726 },
   // { L"avi", AUDACITY_AV_CODEC_ID_MP2 }, //Bug 59
   { L"avi", AUDACITY_AV_CODEC_ID_MP3 },
#if LIBAVCODEC_VERSION_MAJOR < 58
   { L"avi", AUDACITY_AV_CODEC_ID_VOXWARE },
#endif
   { L"avi", AUDACITY_AV_CODEC_ID_AAC },
   { L"avi", AUDACITY_AV_CODEC_ID_WMAV1 },
   { L"avi", AUDACITY_AV_CODEC_ID_WMAV2 },
   { L"avi", AUDACITY_AV_CODEC_ID_WMAPRO },
   { L"avi", AUDACITY_AV_CODEC_ID_ADPCM_CT },
   { L"avi", AUDACITY_AV_CODEC_ID_ATRAC3 },
   { L"avi", AUDACITY_AV_CODEC_ID_IMC },
   { L"avi", AUDACITY_AV_CODEC_ID_AC3 },
   { L"avi", AUDACITY_AV_CODEC_ID_DTS },
   { L"avi", AUDACITY_AV_CODEC_ID_FLAC },
   { L"avi", AUDACITY_AV_CODEC_ID_ADPCM_SWF },
   { L"avi", AUDACITY_AV_CODEC_ID_VORBIS },

   { L"crc", AUDACITY_AV_CODEC_ID_NONE },

   { L"dv", AUDACITY_AV_CODEC_ID_PCM_S16LE },

   { L"ffm", AUDACITY_AV_CODEC_ID_NONE },

   { L"flv", AUDACITY_AV_CODEC_ID_MP3 },
   { L"flv", AUDACITY_AV_CODEC_ID_PCM_S8 },
   { L"flv", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   { L"flv", AUDACITY_AV_CODEC_ID_PCM_S16LE },
   { L"flv", AUDACITY_AV_CODEC_ID_ADPCM_SWF },
   { L"flv", AUDACITY_AV_CODEC_ID_AAC },
   { L"flv", AUDACITY_AV_CODEC_ID_NELLYMOSER },

   { L"framecrc", AUDACITY_AV_CODEC_ID_NONE },

   { L"gxf", AUDACITY_AV_CODEC_ID_PCM_S16LE },

   { L"matroska", AUDACITY_AV_CODEC_ID_PCM_S16LE },
   { L"matroska", AUDACITY_AV_CODEC_ID_PCM_U8 },
   { L"matroska", AUDACITY_AV_CODEC_ID_PCM_S24LE },
   { L"matroska", AUDACITY_AV_CODEC_ID_PCM_S32LE },
   { L"matroska", AUDACITY_AV_CODEC_ID_ADPCM_MS },
   { L"matroska", AUDACITY_AV_CODEC_ID_PCM_ALAW },
   { L"matroska", AUDACITY_AV_CODEC_ID_PCM_MULAW },
   { L"matroska", AUDACITY_AV_CODEC_ID_WMAVOICE },
   { L"matroska", AUDACITY_AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"matroska", AUDACITY_AV_CODEC_ID_ADPCM_YAMAHA },
   { L"matroska", AUDACITY_AV_CODEC_ID_TRUESPEECH },
   { L"matroska", AUDACITY_AV_CODEC_ID_GSM_MS },
   { L"matroska", AUDACITY_AV_CODEC_ID_ADPCM_G726 },
   // { L"matroska", AUDACITY_AV_CODEC_ID_MP2 }, // Bug 59
   { L"matroska", AUDACITY_AV_CODEC_ID_MP3 },
#if LIBAVCODEC_VERSION_MAJOR < 58
   { L"matroska", AUDACITY_AV_CODEC_ID_VOXWARE },
#endif
   { L"matroska", AUDACITY_AV_CODEC_ID_AAC },
   { L"matroska", AUDACITY_AV_CODEC_ID_WMAV1 },
   { L"matroska", AUDACITY_AV_CODEC_ID_WMAV2 },
   { L"matroska", AUDACITY_AV_CODEC_ID_WMAPRO },
   { L"matroska", AUDACITY_AV_CODEC_ID_ADPCM_CT },
   { L"matroska", AUDACITY_AV_CODEC_ID_ATRAC3 },
   { L"matroska", AUDACITY_AV_CODEC_ID_IMC },
   { L"matroska", AUDACITY_AV_CODEC_ID_AC3 },
   { L"matroska", AUDACITY_AV_CODEC_ID_DTS },
   { L"matroska", AUDACITY_AV_CODEC_ID_FLAC },
   { L"matroska", AUDACITY_AV_CODEC_ID_ADPCM_SWF },
   { L"matroska", AUDACITY_AV_CODEC_ID_VORBIS },

   { L"mmf", AUDACITY_AV_CODEC_ID_ADPCM_YAMAHA },

   { L"mov", AUDACITY_AV_CODEC_ID_PCM_S32BE }, //mov
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_S32LE },
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_S24BE },
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_S24LE },
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_S16LE },
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_S8 },
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_U8 },
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_MULAW },
   { L"mov", AUDACITY_AV_CODEC_ID_PCM_ALAW },
   { L"mov", AUDACITY_AV_CODEC_ID_ADPCM_IMA_QT },
   { L"mov", AUDACITY_AV_CODEC_ID_MACE3 },
   { L"mov", AUDACITY_AV_CODEC_ID_MACE6 },
   { L"mov", AUDACITY_AV_CODEC_ID_MP3 },
   { L"mov", AUDACITY_AV_CODEC_ID_AAC },
   { L"mov", AUDACITY_AV_CODEC_ID_AMR_NB },
   { L"mov", AUDACITY_AV_CODEC_ID_AMR_WB },
   { L"mov", AUDACITY_AV_CODEC_ID_GSM },
   { L"mov", AUDACITY_AV_CODEC_ID_ALAC },
   { L"mov", AUDACITY_AV_CODEC_ID_QCELP },
   { L"mov", AUDACITY_AV_CODEC_ID_QDM2 },
   { L"mov", AUDACITY_AV_CODEC_ID_DVAUDIO },
   { L"mov", AUDACITY_AV_CODEC_ID_WMAV2 },
   { L"mov", AUDACITY_AV_CODEC_ID_ALAC },

   { L"mp4", AUDACITY_AV_CODEC_ID_AAC },
   { L"mp4", AUDACITY_AV_CODEC_ID_QCELP },
   { L"mp4", AUDACITY_AV_CODEC_ID_MP3 },
   { L"mp4", AUDACITY_AV_CODEC_ID_VORBIS },

   { L"psp", AUDACITY_AV_CODEC_ID_AAC },
   { L"psp", AUDACITY_AV_CODEC_ID_QCELP },
   { L"psp", AUDACITY_AV_CODEC_ID_MP3 },
   { L"psp", AUDACITY_AV_CODEC_ID_VORBIS },

   { L"ipod", AUDACITY_AV_CODEC_ID_AAC },
   { L"ipod", AUDACITY_AV_CODEC_ID_QCELP },
   { L"ipod", AUDACITY_AV_CODEC_ID_MP3 },
   { L"ipod", AUDACITY_AV_CODEC_ID_VORBIS },

   { L"3gp", AUDACITY_AV_CODEC_ID_AAC },
   { L"3gp", AUDACITY_AV_CODEC_ID_AMR_NB },
   { L"3gp", AUDACITY_AV_CODEC_ID_AMR_WB },

   { L"3g2", AUDACITY_AV_CODEC_ID_AAC },
   { L"3g2", AUDACITY_AV_CODEC_ID_AMR_NB },
   { L"3g2", AUDACITY_AV_CODEC_ID_AMR_WB },

   { L"mp3", AUDACITY_AV_CODEC_ID_MP3 },

   { L"mpeg", AUDACITY_AV_CODEC_ID_AC3 },
   { L"mpeg", AUDACITY_AV_CODEC_ID_DTS },
   { L"mpeg", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   //{ L"mpeg", AUDACITY_AV_CODEC_ID_MP2 },// Bug 59

   { L"vcd", AUDACITY_AV_CODEC_ID_AC3 },
   { L"vcd", AUDACITY_AV_CODEC_ID_DTS },
   { L"vcd", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   //{ L"vcd", AUDACITY_AV_CODEC_ID_MP2 },// Bug 59

   { L"vob", AUDACITY_AV_CODEC_ID_AC3 },
   { L"vob", AUDACITY_AV_CODEC_ID_DTS },
   { L"vob", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   //{ L"vob", AUDACITY_AV_CODEC_ID_MP2 },// Bug 59

   { L"svcd", AUDACITY_AV_CODEC_ID_AC3 },
   { L"svcd", AUDACITY_AV_CODEC_ID_DTS },
   { L"svcd", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   //{ L"svcd", AUDACITY_AV_CODEC_ID_MP2 },// Bug 59

   { L"dvd", AUDACITY_AV_CODEC_ID_AC3 },
   { L"dvd", AUDACITY_AV_CODEC_ID_DTS },
   { L"dvd", AUDACITY_AV_CODEC_ID_PCM_S16BE },
   //{ L"dvd", AUDACITY_AV_CODEC_ID_MP2 },// Bug 59

   { L"nut", AUDACITY_AV_CODEC_ID_PCM_S16LE },
   { L"nut", AUDACITY_AV_CODEC_ID_PCM_U8 },
   { L"nut", AUDACITY_AV_CODEC_ID_PCM_S24LE },
   { L"nut", AUDACITY_AV_CODEC_ID_PCM_S32LE },
   { L"nut", AUDACITY_AV_CODEC_ID_ADPCM_MS },
   { L"nut", AUDACITY_AV_CODEC_ID_PCM_ALAW },
   { L"nut", AUDACITY_AV_CODEC_ID_PCM_MULAW },
   { L"nut", AUDACITY_AV_CODEC_ID_WMAVOICE },
   { L"nut", AUDACITY_AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"nut", AUDACITY_AV_CODEC_ID_ADPCM_YAMAHA },
   { L"nut", AUDACITY_AV_CODEC_ID_TRUESPEECH },
   { L"nut", AUDACITY_AV_CODEC_ID_GSM_MS },
   { L"nut", AUDACITY_AV_CODEC_ID_ADPCM_G726 },
   //{ L"nut", AUDACITY_AV_CODEC_ID_MP2 },// Bug 59
   { L"nut", AUDACITY_AV_CODEC_ID_MP3 },
 #if LIBAVCODEC_VERSION_MAJOR < 58
   { L"nut", AUDACITY_AV_CODEC_ID_VOXWARE },
 #endif
   { L"nut", AUDACITY_AV_CODEC_ID_AAC },
   { L"nut", AUDACITY_AV_CODEC_ID_WMAV1 },
   { L"nut", AUDACITY_AV_CODEC_ID_WMAV2 },
   { L"nut", AUDACITY_AV_CODEC_ID_WMAPRO },
   { L"nut", AUDACITY_AV_CODEC_ID_ADPCM_CT },
   { L"nut", AUDACITY_AV_CODEC_ID_ATRAC3 },
   { L"nut", AUDACITY_AV_CODEC_ID_IMC },
   { L"nut", AUDACITY_AV_CODEC_ID_AC3 },
   { L"nut", AUDACITY_AV_CODEC_ID_DTS },
   { L"nut", AUDACITY_AV_CODEC_ID_FLAC },
   { L"nut", AUDACITY_AV_CODEC_ID_ADPCM_SWF },
   { L"nut", AUDACITY_AV_CODEC_ID_VORBIS },

   { L"ogg", AUDACITY_AV_CODEC_ID_VORBIS },
   { L"ogg", AUDACITY_AV_CODEC_ID_FLAC },

   { L"ac3", AUDACITY_AV_CODEC_ID_AC3 },

   { L"dts", AUDACITY_AV_CODEC_ID_DTS },

   { L"flac", AUDACITY_AV_CODEC_ID_FLAC },

   { L"RoQ", AUDACITY_AV_CODEC_ID_ROQ_DPCM },

   { L"rm", AUDACITY_AV_CODEC_ID_AC3 },

   { L"swf", AUDACITY_AV_CODEC_ID_MP3 },

   { L"avm2", AUDACITY_AV_CODEC_ID_MP3 },

   { L"voc", AUDACITY_AV_CODEC_ID_PCM_U8 },

   { L"wav", AUDACITY_AV_CODEC_ID_PCM_S16LE },
   { L"wav", AUDACITY_AV_CODEC_ID_PCM_U8 },
   { L"wav", AUDACITY_AV_CODEC_ID_PCM_S24LE },
   { L"wav", AUDACITY_AV_CODEC_ID_PCM_S32LE },
   { L"wav", AUDACITY_AV_CODEC_ID_ADPCM_MS },
   { L"wav", AUDACITY_AV_CODEC_ID_PCM_ALAW },
   { L"wav", AUDACITY_AV_CODEC_ID_PCM_MULAW },
   { L"wav", AUDACITY_AV_CODEC_ID_WMAVOICE },
   { L"wav", AUDACITY_AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"wav", AUDACITY_AV_CODEC_ID_ADPCM_YAMAHA },
   { L"wav", AUDACITY_AV_CODEC_ID_TRUESPEECH },
   { L"wav", AUDACITY_AV_CODEC_ID_GSM_MS },
   { L"wav", AUDACITY_AV_CODEC_ID_ADPCM_G726 },
   //{ L"wav", AUDACITY_AV_CODEC_ID_MP2 }, Bug 59 - It crashes.
   { L"wav", AUDACITY_AV_CODEC_ID_MP3 },
#if LIBAVCODEC_VERSION_MAJOR < 58
   { L"wav", AUDACITY_AV_CODEC_ID_VOXWARE },
#endif
   { L"wav", AUDACITY_AV_CODEC_ID_AAC },
   // { L"wav", AUDACITY_AV_CODEC_ID_WMAV1 },
   // { L"wav", AUDACITY_AV_CODEC_ID_WMAV2 },
   { L"wav", AUDACITY_AV_CODEC_ID_WMAPRO },
   { L"wav", AUDACITY_AV_CODEC_ID_ADPCM_CT },
   { L"wav", AUDACITY_AV_CODEC_ID_ATRAC3 },
   { L"wav", AUDACITY_AV_CODEC_ID_IMC },
   { L"wav", AUDACITY_AV_CODEC_ID_AC3 },
   //{ L"wav", AUDACITY_AV_CODEC_ID_DTS },
   { L"wav", AUDACITY_AV_CODEC_ID_FLAC },
   { L"wav", AUDACITY_AV_CODEC_ID_ADPCM_SWF },
   { L"wav", AUDACITY_AV_CODEC_ID_VORBIS },

   { NULL, AUDACITY_AV_CODEC_ID_NONE }
};

/// AAC profiles
// The FF_PROFILE_* enumeration is defined in the ffmpeg library
// PRL:  I cant find where this preference is used!
ChoiceSetting AACProfiles { L"/FileFormats/FFmpegAACProfile",
   {
      {L"1" /*FF_PROFILE_AAC_LOW*/, XO("LC")},
      {L"0" /*FF_PROFILE_AAC_MAIN*/, XO("Main")},
      // {L"2" /*FF_PROFILE_AAC_SSR*/, XO("SSR")}, //SSR is not supported
      {L"3" /*FF_PROFILE_AAC_LTP*/, XO("LTP")},
   },
   0, // "1"
};

/// List of export types
ExposedFormat ExportFFmpegOptions::fmts[] =
{
   {FMT_M4A,   L"M4A",    L"m4a",  L"ipod", 48,  AV_CANMETA,              true,  XO("M4A (AAC) Files (FFmpeg)"),         AUDACITY_AV_CODEC_ID_AAC,    true},
   {FMT_AC3,   L"AC3",    L"ac3",  L"ac3",  7,   AV_VERSION_INT(0,0,0),   false, XO("AC3 Files (FFmpeg)"),               AUDACITY_AV_CODEC_ID_AC3,    true},
   {FMT_AMRNB, L"AMRNB",  L"amr",  L"amr",  1,   AV_VERSION_INT(0,0,0),   false, XO("AMR (narrow band) Files (FFmpeg)"), AUDACITY_AV_CODEC_ID_AMR_NB, true},
   {FMT_OPUS,  L"OPUS",   L"opus", L"opus", 255, AV_CANMETA,              true,  XO("Opus (OggOpus) Files (FFmpeg)"),    AUDACITY_AV_CODEC_ID_OPUS,   true},
   {FMT_WMA2,  L"WMA",    L"wma",  L"asf",  2,   AV_VERSION_INT(52,53,0), false, XO("WMA (version 2) Files (FFmpeg)"),   AUDACITY_AV_CODEC_ID_WMAV2,  true},
   {FMT_OTHER, L"FFMPEG", L"",     L"",     255, AV_CANMETA,              true,  XO("Custom FFmpeg Export"),             AUDACITY_AV_CODEC_ID_NONE,   true}
};

namespace {

/// Prediction order method - names.
const TranslatableStrings PredictionOrderMethodNames {
   XO("Estimate"),
   XO("2-level"),
   XO("4-level"),
   XO("8-level"),
   XO("Full search"),
   XO("Log search"),
};

}



ExportFFmpegOptions::~ExportFFmpegOptions()
{
}

ExportFFmpegOptions::ExportFFmpegOptions(wxWindow *parent)
:  wxDialogWrapper(parent, wxID_ANY,
            XO("Configure custom FFmpeg options"))
{
   SetName();
   ShuttleGui S(this, eIsCreatingFromPrefs);
   mFFmpeg = FFmpegFunctions::Load();
   //FFmpegLibsInst()->LoadLibs(NULL,true); //Loaded at startup or from Prefs now

   mPresets = std::make_unique<FFmpegPresets>();
   mPresets->GetPresetList(mPresetNames);

   if (mFFmpeg)
   {
      FetchFormatList();
      FetchCodecList();

      PopulateOrExchange(S);

      //Select the format that was selected last time this dialog was closed
      mFormatList->Select( mFormatList->FindString( FFmpegFormat.Read() ) );
      DoOnFormatList();

      //Select the codec that was selected last time this dialog was closed
      auto codec = mFFmpeg->CreateEncoder(FFmpegCodec.Read().ToUTF8());

      if (codec != nullptr)
         mCodecList->Select(mCodecList->FindString(wxString::FromUTF8(codec->GetName())));

      DoOnCodecList();
   }

}

///
///
void ExportFFmpegOptions::FetchFormatList()
{
   if (!mFFmpeg)
      return;

   // Enumerate all output formats
   std::unique_ptr<AVOutputFormatWrapper> ofmt;

   while ((ofmt = mFFmpeg->GetNextOutputFormat(ofmt.get()))!=NULL)
   {
      // Any audio-capable format has default audio codec.
      // If it doesn't, then it doesn't supports any audio codecs
      if (ofmt->GetAudioCodec() != AUDACITY_AV_CODEC_ID_NONE)
      {
         mFormatNames.push_back(wxString::FromUTF8(ofmt->GetName()));
         mFormatLongNames.push_back(wxString::Format(L"%s - %s",mFormatNames.back(),wxString::FromUTF8(ofmt->GetLongName())));
      }
   }
   // Show all formats
   mShownFormatNames = mFormatNames;
   mShownFormatLongNames =  mFormatLongNames;
}

///
///
void ExportFFmpegOptions::FetchCodecList()
{
   if (!mFFmpeg)
      return;
   // Enumerate all codecs
   std::unique_ptr<AVCodecWrapper> codec;
   while ((codec = mFFmpeg->GetNextCodec(codec.get()))!=NULL)
   {
      // We're only interested in audio and only in encoders
      if (codec->IsAudio() && mFFmpeg->av_codec_is_encoder(codec->GetWrappedValue()))
      {
         // MP2 Codec is broken.  Don't allow it.
         if( codec->GetId() == mFFmpeg->GetAVCodecID(AUDACITY_AV_CODEC_ID_MP2))
            continue;

         mCodecNames.push_back(wxString::FromUTF8(codec->GetName()));
         mCodecLongNames.push_back(wxString::Format(L"%s - %s",mCodecNames.back(),wxString::FromUTF8(codec->GetLongName())));
      }
   }
   // Show all codecs
   mShownCodecNames = mCodecNames;
   mShownCodecLongNames = mCodecLongNames;
}

BoolSetting FFmpegBitReservoir{
   L"/FileFormats/FFmpegBitReservoir",     true};
BoolSetting FFmpegUseLPC{
   L"/FileFormats/FFmpegUseLPC",           true};
BoolSetting FFmpegVariableBlockLen{
   L"/FileFormats/FFmpegVariableBlockLen", true};

IntSetting FFmpegBitRate{
   L"/FileFormats/FFmpegBitRate",          0 };
IntSetting FFmpegCompLevel{
   L"/FileFormats/FFmpegCompLevel",        0 };
IntSetting FFmpegCutOff{
   L"/FileFormats/FFmpegCutOff",           0 };
IntSetting FFmpegFrameSize{
   L"/FileFormats/FFmpegFrameSize",        0 };
IntSetting FFmpegLPCCoefPrec{
   L"/FileFormats/FFmpegLPCCoefPrec",      0 };
IntSetting FFmpegMaxPartOrder{
   L"/FileFormats/FFmpegMaxPartOrder",     -1 };
IntSetting FFmpegMaxPredOrder{
   L"/FileFormats/FFmpegMaxPredOrder",     -1};
IntSetting FFmpegMinPartOrder{
   L"/FileFormats/FFmpegMinPartOrder",     -1};
IntSetting FFmpegMinPredOrder{
   L"/FileFormats/FFmpegMinPredOrder",     -1 };
IntSetting FFmpegMuxRate{
   L"/FileFormats/FFmpegMuxRate",          0 };
IntSetting FFmpegPacketSize{
   L"/FileFormats/FFmpegPacketSize",       0 };
IntSetting FFmpegPredictionOrderMethod{
   L"/FileFormats/FFmpegPredOrderMethod",  4 }; // Full search
IntSetting FFmpegQuality{
   L"/FileFormats/FFmpegQuality",          0 };
IntSetting FFmpegSampleRate{
   L"/FileFormats/FFmpegSampleRate",       0 };

StringSetting FFmpegLanguage{ L"/FileFormats/FFmpegLanguage" };
StringSetting FFmpegTag{ L"/FileFormats/FFmpegTag" };

// Used only as memory for this dialog
static StringSetting FFmpegPreset{ L"/FileFormats/FFmpegPreset", L"" };

///
///
void ExportFFmpegOptions::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   // A function-factory
   const auto forCodecs = [this]( std::vector<AudacityAVCodecIDValue> ids )
      -> DialogDefinition::BaseItem::Test {
      return [this, ids]{
         int sel = 0;
         //const auto sel = FindSelectedCodec();
         if ( sel < 0 )
            return false;

//         const auto cdc =
  //          avcodec_find_encoder_by_name( mCodecNames[ sel ].ToUTF8() );
//         if ( !cdc )
  //          return false;

//         const auto end = ids.end();
  //       return end != std::find_if( ids.begin(), end,
    //        [cdc](const AudacityAVCodecIDValue &id){ return id == cdc->id; } );
         return true;
      };
   };
   
   // Another function-factory
   const auto forFormats = [this]( std::vector<const char *> formats )
      -> DialogDefinition::BaseItem::Test {
      wxArrayString strings;
      for ( const auto format : formats )
         strings.push_back( wxString::FromUTF8( format ) );
      return [this, strings]{
         int sel = 0;
         //const auto sel = FindSelectedFormat();
         if ( sel < 0 )
            return false;

         const auto end = strings.end();
         return end != std::find( strings.begin(), end, mFormatNames[ sel ] );
      };
   };
   
   const auto forFLAC = forCodecs( {
      AUDACITY_AV_CODEC_ID_FLAC,
   } );

   const auto forMuxPacket = forFormats( {
      "mpeg",
      "vcd",
      "vob",
      "svcd",
      "dvd",
   } );

   const auto forQualityCutoff = forCodecs( {
      AUDACITY_AV_CODEC_ID_AAC,
      AUDACITY_AV_CODEC_ID_MP3,
      AUDACITY_AV_CODEC_ID_VORBIS,
   } );

   S.StartVerticalLay(1);
   S.StartMultiColumn(1, wxEXPAND);
   {
      S.SetStretchyRow(3);
      S.StartMultiColumn(7, wxEXPAND);
      {
         S.SetStretchyCol(1);

         mPresetCombo =
         S
            .Id(FEPresetID)
            .AddCombo(XXO("Preset:"), FFmpegPreset.Read(), mPresetNames);

         S
            .Action( [this]{ OnLoadPreset(); } )
            .AddButton(XXO("Load Preset"));

         S
            .Action( [this]{ OnSavePreset(); } )
            .AddButton(XXO("Save Preset"));

         S
            .Action( [this]{ OnDeletePreset(); } )
            .AddButton(XXO("Delete Preset"));

         S
            .Action( [this]{ OnImportPresets(); } )
            .AddButton(XXO("Import Presets"));

         S
            .Action( [this]{ OnExportPresets(); } )
            .AddButton(XXO("Export Presets"));
      }
      S.EndMultiColumn();
      S.StartMultiColumn(4, wxALIGN_LEFT);
      {
         S.SetStretchyCol(1);
         S.SetStretchyCol(3);

         S
            .AddFixedText(XO("Format:"));

         mFormatName =
         S
            .AddVariableText( {} );

         S
            /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
            .AddFixedText(XO("Codec:"));

         mCodecName =
         S
            .AddVariableText( {} );
      }
      S.EndMultiColumn();
      S.AddVariableText(XO(
"Not all formats and codecs are compatible. Nor are all option combinations compatible with all codecs."),
         false);
      S.StartMultiColumn(2, wxEXPAND);
      {
         S.StartMultiColumn(2, wxEXPAND);
         {
            S.SetStretchyRow(1);
   
            S
               .Action( [this]{ OnAllFormats(); } )
               .AddButton(XXO("Show All Formats"));
   
            S
               .Action( [this]{ OnAllCodecs(); } )
               .AddButton(XXO("Show All Codecs"));
   
            mFormatList =
            S
               .Id(FEFormatID)
               .AddListBox(mFormatNames);

            mFormatList->DeselectAll();

            mCodecList =
            S
               .Id(FECodecID)
               .AddListBox(mCodecNames);

            mCodecList->DeselectAll();
         }
         S.EndMultiColumn();
         S.StartVerticalLay();
         {
            //S.StartScroller( );
            S.SetBorder( 3 );
            S.StartStatic(XO("General Options"), 0);
            {
               S.StartMultiColumn(8, wxEXPAND);
               {
                  S
                     .Id(FELanguageID)
                     .Text({ {}, {}, XO("ISO 639 3-letter language code\nOptional\nempty - automatic") })
                     .Enable( forFormats( {
                        "matroska",
                        "mov",
                        "3gp",
                        "mp4",
                        "psp",
                        "3g2",
                        "ipod",
                        "mpegts",
                     } ) )
                     .Target( FFmpegLanguage )
                     .AddTextBox(XXO("Language:"), {}, 9);

                  S.AddSpace( 20,0 );

                  S
                     .AddVariableText(XO("Bit Reservoir"));

                  S
                     .Id(FEBitReservoirID)
                     .Enable( forCodecs( {
                        AUDACITY_AV_CODEC_ID_MP3,
                        AUDACITY_AV_CODEC_ID_WMAV1,
                        AUDACITY_AV_CODEC_ID_WMAV2,
                     } ) )
                     .Target( FFmpegBitReservoir )
                     .AddCheckBox( {} );

                  S.AddSpace( 20,0 );

                  S
                     .AddVariableText(XO("VBL"));

                  S
                     .Id(FEVariableBlockLenID)
                     .Enable( forCodecs( {
                        AUDACITY_AV_CODEC_ID_WMAV1,
                        AUDACITY_AV_CODEC_ID_WMAV2,
                     } ) )
                     .Target( FFmpegVariableBlockLen )
                     .AddCheckBox( {} );
               }
               S.EndMultiColumn();
               S.StartMultiColumn(4, wxALIGN_LEFT);
               {
                  S
                     .Id(FETagID)
                     /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
                     .Text({ {}, {}, XO("Codec tag (FOURCC)\nOptional\nempty - automatic") })
                     .Target( FFmpegTag )
                     .AddTextBox(XXO("Tag:"), {}, 4);

                  S
                     .Id(FEBitrateID)
                     .Text({ {}, {}, XO("Bit Rate (bits/second) - influences the resulting file size and quality\nSome codecs may only accept specific values (128k, 192k, 256k etc)\n0 - automatic\nRecommended - 192000") })
                     .Target( FFmpegBitRate )
                     .AddSpinCtrl(XXO("Bit Rate:"), 0, 1000000, 0);

                  S
                     .Id(FEQualityID)
                     .Text({ {}, {}, XO("Overall quality, used differently by different codecs\nRequired for vorbis\n0 - automatic\n-1 - off (use bitrate instead)") })
                     .Enable( forQualityCutoff )
                     .Target( FFmpegQuality )
                     .AddSpinCtrl(XXO("Quality:"), 0, 500, -1);

                  S
                     .Id(FESampleRateID)
                     .Text({ {}, {}, XO("Sample rate (Hz)\n0 - don't change sample rate") })
                     .Target( FFmpegSampleRate )
                     .AddSpinCtrl(XXO("Sample Rate:"), 0, 200000, 0);

                  S
                     .Id(FECutoffID)
                     .Enable( forQualityCutoff )
                     .Text({ {}, {}, XO("Audio cutoff bandwidth (Hz)\nOptional\n0 - automatic") })
                     .Target( FFmpegCutOff )
                     .AddSpinCtrl(XXO("Cutoff:"), 0, 10000000, 0);

                  // PRL:  As commented elsewhere, this preference does nothing
                  S
                     .Id(FEProfileID)
                     .Text({ {}, {}, XO("AAC Profile\nLow Complexity - default\nMost players won't play anything other than LC") })
                     .Enable( forCodecs( {
                        AUDACITY_AV_CODEC_ID_AAC,
                     } ) )
                     .MinSize( { 100, -1 } )
                     .Target( AACProfiles )
                     .AddChoice( XXO("Profile:") );
               }
               S.EndMultiColumn();
            }
            S.EndStatic();
            S.StartStatic(XO("FLAC options"),0);
            {
               S.StartMultiColumn(4, wxALIGN_LEFT);
               {
                  S
                     .Id(FECompLevelID)
                     .Text({ {}, {}, XO("Compression level\nRequired for FLAC\n-1 - automatic\nmin - 0 (fast encoding, large output file)\nmax - 10 (slow encoding, small output file)") })
                     .Enable( forFLAC )
                     .Target( FFmpegCompLevel )
                     .AddSpinCtrl(XXO("Compression:"), 0, 10, -1);

                  S
                     .Id(FEFrameSizeID)
                     .Enable( forFLAC )
                     .Text({ {}, {}, XO("Frame size\nOptional\n0 - default\nmin - 16\nmax - 65535") })
                     .Target( FFmpegFrameSize )
                     .AddSpinCtrl(XXO("Frame:"), 0, 65535, 0);

                  S
                     .Id(FELPCCoeffsID)
                     .Text({ {}, {}, XO("LPC coefficients precision\nOptional\n0 - default\nmin - 1\nmax - 15") })
                     .Enable( forFLAC )
                     .Target( FFmpegLPCCoefPrec )
                     .AddSpinCtrl(XXO("LPC"), 0, 15, 0);

                  S
                     .Id(FEPredOrderID)
                     .Text({ {}, {}, XO("Prediction Order Method\nEstimate - fastest, lower compression\nLog search - slowest, best compression\nFull search - default") })
                     .MinSize( { 100, -1 } )
                     .Enable( forFLAC )
                     .Target( NumberChoice(
                        FFmpegPredictionOrderMethod, PredictionOrderMethodNames ) )
                     .AddChoice( XXO("PdO Method:") );

                  S
                     .Id(FEMinPredID)
                     .Text({ {}, {}, XO("Minimal prediction order\nOptional\n-1 - default\nmin - 0\nmax - 32 (with LPC) or 4 (without LPC)") })
                     .Enable( forFLAC )
                     .Target( FFmpegMinPredOrder )
                     .AddSpinCtrl(XXO("Min. PdO"), 0, 32, -1);

                  S
                     .Id(FEMaxPredID)
                     .Text({ {}, {}, XO("Maximal prediction order\nOptional\n-1 - default\nmin - 0\nmax - 32 (with LPC) or 4 (without LPC)") })
                     .Enable( forFLAC )
                     .Target( FFmpegMaxPredOrder )
                     .AddSpinCtrl(XXO("Max. PdO"), 0, 32, -1);

                  S
                     .Id(FEMinPartOrderID)
                     .Text({ {}, {}, XO("Minimal partition order\nOptional\n-1 - default\nmin - 0\nmax - 8") })
                     .Enable( forFLAC )
                     .Target( FFmpegMinPartOrder )
                     .AddSpinCtrl(XXO("Min. PtO"), 0, 8, -1);

                  S
                     .Id(FEMaxPartOrderID)
                     .Text({ {}, {}, XO("Maximal partition order\nOptional\n-1 - default\nmin - 0\nmax - 8") })
                     .Enable( forFLAC )
                     .Target( FFmpegMaxPartOrder )
                     .AddSpinCtrl(XXO("Max. PtO"), 0, 8, -1);

                  S
                     /* i18n-hint:  Abbreviates "Linear Predictive Coding",
                        but this text needs to be kept very short */
                     .AddVariableText(XO("Use LPC"));

                  // PRL:  This preference is not used anywhere!
                  S
                     .Id(FEUseLPCID)
                     .Enable( forFLAC )
                     .Target( FFmpegUseLPC )
                     .AddCheckBox( {} );
               }
               S.EndMultiColumn();
            }
            S.EndStatic();
            S.StartStatic(XO("MPEG container options"),0);
            {
               S.StartMultiColumn(4, wxALIGN_LEFT);
               {
                  S
                     .Id(FEMuxRateID)
                     .Text({ {}, {}, XO("Maximum bit rate of the multiplexed stream\nOptional\n0 - default") })
                     .Enable( forMuxPacket )
                     .Target( FFmpegMuxRate )
                        /* i18n-hint: 'mux' is short for multiplexor, a device that selects between several inputs
                          'Mux Rate' is a parameter that has some bearing on compression ratio for MPEG
                          it has a hard to predict effect on the degree of compression */
                     .AddSpinCtrl(XXO("Mux Rate:"), 0, 10000000, 0);

                  S
                     .Id(FEPacketSizeID)
                     /* i18n-hint: 'Packet Size' is a parameter that has some bearing on compression ratio for MPEG
                       compression.  It measures how big a chunk of audio is compressed in one piece. */
                     .Text({ {}, {}, XO("Packet size\nOptional\n0 - default") })
                     .Enable( forMuxPacket )
                     .Target( FFmpegPacketSize )
                     /* i18n-hint: 'Packet Size' is a parameter that has some bearing on compression ratio for MPEG
                       compression.  It measures how big a chunk of audio is compressed in one piece. */
                     .AddSpinCtrl(XXO("Packet Size:"), 0, 10000000, 0);
               }
               S.EndMultiColumn();
            }
            S.EndStatic();
            //S.EndScroller();
            S.SetBorder( 5 );

            S
               .AddStandardButtons( eCancelButton, {
                  S.Item( eOkButton ).Action( [this]{ OnOK(); } ),
                  S.Item( eHelpButton ).Action( [this]{ OnGetURL(); } )
               });
         }
         S.EndVerticalLay();
      }
      S.EndMultiColumn();
   }
   S.EndMultiColumn();
   S.EndVerticalLay();

   Layout();
   Fit();
   SetMinSize(GetSize());
   Center();

   return;
}

///
///
void ExportFFmpegOptions::FindSelectedFormat(wxString **name, wxString **longname)
{
   // Get current selection
   wxArrayInt selections;
   int n = mFormatList->GetSelections(selections);
   if (n <= 0) return;

   // Get selected format short name
   wxString selfmt = mFormatList->GetString(selections[0]);

   // Find its index
   int nFormat = make_iterator_range( mFormatNames ).index( selfmt );
   if (nFormat == wxNOT_FOUND) return;

   // Return short name and description
   if (name != NULL) *name = &mFormatNames[nFormat];
   if (longname != NULL) *longname = &mFormatLongNames[nFormat];
   return;
}
///
///
void ExportFFmpegOptions::FindSelectedCodec(wxString **name, wxString **longname)
{
   // Get current selection
   wxArrayInt selections;
   int n = mCodecList->GetSelections(selections);
   if (n <= 0) return;

   // Get selected codec short name
   wxString selcdc = mCodecList->GetString(selections[0]);

   // Find its index
   int nCodec = make_iterator_range( mCodecNames ).index( selcdc );
   if (nCodec == wxNOT_FOUND) return;

   // Return short name and description
   if (name != NULL) *name = &mCodecNames[nCodec];
   if (longname != NULL) *longname = &mCodecLongNames[nCodec];
}

///
///
int ExportFFmpegOptions::FetchCompatibleCodecList(const wxChar *fmt, AudacityAVCodecID id)
{
   const auto ffmpegId = mFFmpeg->GetAVCodecID(id);

   // By default assume that id is not in the list
   int index = -1;
   // By default no codecs are compatible (yet)
   mShownCodecNames.clear();
   mShownCodecLongNames.clear();
   // Clear the listbox
   mCodecList->Clear();
   // Zero - format is not found at all
   int found = 0;
   wxString str(fmt);
   for (int i = 0; CompatibilityList[i].fmt != NULL; i++)
   {
      if (str == CompatibilityList[i].fmt)
      {
         // Format is found in the list
         found = 1;
         if (CompatibilityList[i].codec.value == AUDACITY_AV_CODEC_ID_NONE)
         {
            // Format is found in the list and it is compatible with AUDACITY_AV_CODEC_ID_NONE (means that it is compatible to anything)
            found = 2;
            break;
         }
         // Find the codec, that is claimed to be compatible
         std::unique_ptr<AVCodecWrapper> codec = mFFmpeg->CreateEncoder(mFFmpeg->GetAVCodecID(CompatibilityList[i].codec));
         // If it exists, is audio and has encoder
         if (codec != NULL && codec->IsAudio() && mFFmpeg->av_codec_is_encoder(codec->GetWrappedValue()))
         {
            // If it was selected - remember its NEW index
            if ((ffmpegId >= 0) && codec->GetId() == ffmpegId)
               index = mShownCodecNames.size();

            mShownCodecNames.push_back(wxString::FromUTF8(codec->GetName()));
            mShownCodecLongNames.push_back(wxString::Format(L"%s - %s",mShownCodecNames.back(),wxString::FromUTF8(codec->GetLongName())));
         }
      }
   }
   // All codecs are compatible with this format
   if (found == 2)
   {
      std::unique_ptr<AVCodecWrapper> codec;
      while ((codec = mFFmpeg->GetNextCodec(codec.get()))!=NULL)
      {
         if (codec->IsAudio() && mFFmpeg->av_codec_is_encoder(codec->GetWrappedValue()))
         {
            // MP2 is broken.
            if( codec->GetId() == mFFmpeg->GetAVCodecID(AUDACITY_AV_CODEC_ID_MP2) )
               continue;

            if (! make_iterator_range( mShownCodecNames )
               .contains( wxString::FromUTF8(codec->GetName()) ) )
            {
               if ((ffmpegId >= 0) && codec->GetId() == ffmpegId)
                  index = mShownCodecNames.size();

               mShownCodecNames.push_back(wxString::FromUTF8(codec->GetName()));
               mShownCodecLongNames.push_back(wxString::Format(L"%s - %s",mShownCodecNames.back(),wxString::FromUTF8(codec->GetLongName())));
            }
         }
      }
   }
   // Format is not found - find format in libavformat and add its default audio codec
   // This allows us to provide limited support for NEW formats without modifying the compatibility list
   else if (found == 0)
   {
      wxCharBuffer buf = str.ToUTF8();
      auto format = mFFmpeg->GuessOutputFormat(buf, nullptr, nullptr);

      if (format != nullptr)
      {
         auto codec = mFFmpeg->CreateEncoder(format->GetAudioCodec());

         if (
            codec != nullptr && codec->IsAudio() && mFFmpeg->av_codec_is_encoder(codec->GetWrappedValue()))
         {
            if ((ffmpegId >= 0) && codec->GetId() == ffmpegId)
               index = mShownCodecNames.size();

            mShownCodecNames.push_back(wxString::FromUTF8(codec->GetName()));
            mShownCodecLongNames.push_back(wxString::Format(L"%s - %s",mShownCodecNames.back(),wxString::FromUTF8(codec->GetLongName())));
         }
      }
   }
   // Show NEW codec list
   mCodecList->Append(mShownCodecNames);

   return index;
}

///
///
int ExportFFmpegOptions::FetchCompatibleFormatList(
   AudacityAVCodecID id, wxString* selfmt)
{
   int index = -1;
   mShownFormatNames.clear();
   mShownFormatLongNames.clear();
   mFormatList->Clear();
   std::unique_ptr<AVOutputFormatWrapper> ofmt;

   wxArrayString FromList;
   // Find all formats compatible to this codec in compatibility list
   for (int i = 0; CompatibilityList[i].fmt != NULL; i++)
   {
      if (CompatibilityList[i].codec == id || (CompatibilityList[i].codec.value == AUDACITY_AV_CODEC_ID_NONE) )
      {
         if ((selfmt != NULL) && (*selfmt == CompatibilityList[i].fmt)) index = mShownFormatNames.size();
         FromList.push_back(CompatibilityList[i].fmt);
         mShownFormatNames.push_back(CompatibilityList[i].fmt);
         auto tofmt = mFFmpeg->GuessOutputFormat(
            wxString(CompatibilityList[i].fmt).ToUTF8(), nullptr, nullptr);

         if (tofmt != NULL)
         {
            mShownFormatLongNames.push_back(wxString::Format(
               L"%s - %s", CompatibilityList[i].fmt,
               wxString::FromUTF8(tofmt->GetLongName())));
         }
      }
   }
   bool found = false;
   if (selfmt != NULL)
   {
      for (int i = 0; CompatibilityList[i].fmt != NULL; i++)
      {
         if (*selfmt == CompatibilityList[i].fmt)
         {
            found = true;
            break;
         }
      }
   }
   // Format was in the compatibility list
   if (found)
   {
      // Find all formats which have this codec as default and which are not in the list yet and add them too
      while ((ofmt = mFFmpeg->GetNextOutputFormat(ofmt.get()))!=NULL)
      {
         if (ofmt->GetAudioCodec() == mFFmpeg->GetAVCodecID(id))
         {
            wxString ofmtname = wxString::FromUTF8(ofmt->GetName());
            found = false;
            for (unsigned int i = 0; i < FromList.size(); i++)
            {
               if (ofmtname == FromList[i])
               {
                  found = true;
                  break;
               }
            }
            if (!found)
            {
               if ((selfmt != NULL) &&
                  (*selfmt == wxString::FromUTF8(ofmt->GetName())))
                  index = mShownFormatNames.size();

               mShownFormatNames.push_back(wxString::FromUTF8(ofmt->GetName()));

               mShownFormatLongNames.push_back(wxString::Format(
                  L"%s - %s", mShownFormatNames.back(),
                  wxString::FromUTF8(ofmt->GetLongName())));
            }
         }
      }
   }
   mFormatList->Append(mShownFormatNames);
   return index;
}

///
///
void ExportFFmpegOptions::OnDeletePreset()
{
   wxComboBox *preset = dynamic_cast<wxComboBox*>(FindWindowById(FEPresetID,this));
   wxString presetname = preset->GetValue();
   if (presetname.empty())
   {
      AudacityMessageBox( XO("You can't delete a preset without name") );
      return;
   }

   auto query = XO("Delete preset '%s'?").Format( presetname );
   int action = AudacityMessageBox(
      query,
      XO("Confirm Deletion"),
      wxYES_NO | wxCENTRE);
   if (action == wxNO) return;

   mPresets->DeletePreset(presetname);
   long index = preset->FindString(presetname);
   preset->SetValue(wxEmptyString);
   preset->Delete(index);
   mPresetNames.erase(
      std::find( mPresetNames.begin(), mPresetNames.end(), presetname )
   );
}

///
///
void ExportFFmpegOptions::OnSavePreset()
{  const bool kCheckForOverwrite = true;
   SavePreset(kCheckForOverwrite);
}

// Return false if failed to save.
bool ExportFFmpegOptions::SavePreset(bool bCheckForOverwrite)
{
   wxComboBox *preset = dynamic_cast<wxComboBox*>(FindWindowById(FEPresetID,this));
   wxString name = preset->GetValue();
   if (name.empty())
   {
      AudacityMessageBox( XO("You can't save a preset without a name") );
      return false;
   }
   if( bCheckForOverwrite && !mPresets->OverwriteIsOk(name))
      return false;
   if( !mPresets->SavePreset(this,name) )
      return false;
   int index = mPresetNames.Index(name,false);
   if (index == -1)
   {
      mPresetNames.push_back(name);
      mPresetCombo->Clear();
      mPresetCombo->Append(mPresetNames);
      mPresetCombo->Select(mPresetNames.Index(name,false));
   }
   return true;
}

///
///
void ExportFFmpegOptions::OnLoadPreset()
{
   wxComboBox *preset = dynamic_cast<wxComboBox*>(FindWindowById(FEPresetID,this));
   wxString presetname = preset->GetValue();

   mShownFormatNames = mFormatNames;
   mShownFormatLongNames = mFormatLongNames;
   mFormatList->Clear();
   mFormatList->Append(mFormatNames);

   mShownCodecNames = mCodecNames;
   mShownCodecLongNames = mCodecLongNames;
   mCodecList->Clear();
   mCodecList->Append(mCodecNames);

   mPresets->LoadPreset(this,presetname);

   DoOnFormatList();
   DoOnCodecList();
}

static const FileNames::FileTypes &FileTypes()
{
   static const FileNames::FileTypes result{
      FileNames::XMLFiles, FileNames::AllFiles };
   return result;
};

///
///
void ExportFFmpegOptions::OnImportPresets()
{
   wxString path;
   FileDialogWrapper dlg(this,
      XO("Select xml file with presets to import"),
      gPrefs->Read(L"/FileFormats/FFmpegPresetDir"),
      wxEmptyString,
      FileTypes(),
      wxFD_OPEN);
   if (dlg.ShowModal() == wxID_CANCEL) return;
   path = dlg.GetPath();
   mPresets->ImportPresets(path);
   mPresets->GetPresetList(mPresetNames);
   mPresetCombo->Clear();
   mPresetCombo->Append(mPresetNames);
}

///
///
void ExportFFmpegOptions::OnExportPresets()
{
   const bool kCheckForOverwrite = true;
   // Bug 1180 save any pending preset before exporting the lot.
   // If saving fails, don't try to export.
   if( !SavePreset(!kCheckForOverwrite) )
      return;

   wxArrayString presets;
   mPresets->GetPresetList( presets);
   if( presets.Count() < 1)
   {
      AudacityMessageBox( XO("No presets to export") );
      return;
   }

   wxString path;
   FileDialogWrapper dlg(this,
      XO("Select xml file to export presets into"),
      gPrefs->Read(L"/FileFormats/FFmpegPresetDir"),
      wxEmptyString,
      FileTypes(),
      wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
   if (dlg.ShowModal() == wxID_CANCEL) return;
   path = dlg.GetPath();
   mPresets->ExportPresets(path);
}

///
///
void ExportFFmpegOptions::OnAllFormats()
{
   mShownFormatNames = mFormatNames;
   mShownFormatLongNames = mFormatLongNames;
   mFormatList->Clear();
   mFormatList->Append(mFormatNames);
}

///
///
void ExportFFmpegOptions::OnAllCodecs()
{
   mShownCodecNames = mCodecNames;
   mShownCodecLongNames = mCodecLongNames;
   mCodecList->Clear();
   mCodecList->Append(mCodecNames);
}

/// ReportIfBadCombination will trap
/// bad combinations of format and codec and report
/// using a message box.
/// We may later extend it to catch bad parameters too.
/// @return true iff a bad combination was reported
/// At the moment we don't trap unrecognised format
/// or codec.  (We do not expect them to happen ever).
bool ExportFFmpegOptions::ReportIfBadCombination()
{
   wxString *selcdc = nullptr;
   wxString* selcdclong = nullptr;

   FindSelectedCodec(&selcdc, &selcdclong);

   if (selcdc == nullptr)
      return false; // unrecognised codec. Treated as OK

   auto cdc = mFFmpeg->CreateEncoder(selcdc->ToUTF8());

   if (cdc == nullptr)
      return false; // unrecognised codec. Treated as OK

   wxString* selfmt = nullptr;
   wxString* selfmtlong = nullptr;

   FindSelectedFormat(&selfmt, &selfmtlong);

   if (selfmt == nullptr)
      return false; // unrecognised format; Treated as OK

   // This is intended to test for illegal combinations.
   // However, the list updating now seems to be working correctly
   // making it impossible to select illegal combinations
   bool bFound = false;
   for (int i = 0; CompatibilityList[i].fmt != NULL; i++)
   {
      if (*selfmt == CompatibilityList[i].fmt)
      {
         if (CompatibilityList[i].codec == mFFmpeg->GetAudacityCodecID(cdc->GetId()) || (CompatibilityList[i].codec == AUDACITY_AV_CODEC_ID_NONE) ){
            bFound = true;
            break;
         }
      }
   }

   // We can put extra code in here, to disallow combinations
   // We could also test for illegal parameters, and deliver
   // custom error messages in that case.
   // The below would make AAC codec disallowed.
   //if( cdc->id == AUDACITY_AV_CODEC_ID_AAC)
   //   bFound = false;

   // Valid combination was found, so no reporting.
   if( bFound )
      return false;

   AudacityMessageBox(
      /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
      XO("Format %s is not compatible with codec %s.")
         .Format( *selfmt, *selcdc ),
      /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
      XO("Incompatible format and codec"));

   return true;
}

void ExportFFmpegOptions::DoOnFormatList()
{
   wxString *selfmt = NULL;
   wxString *selfmtlong = NULL;
   FindSelectedFormat(&selfmt, &selfmtlong);
   if (selfmt == NULL)
   {
      return;
   }

   wxString *selcdc = NULL;
   wxString *selcdclong = NULL;
   FindSelectedCodec(&selcdc, &selcdclong);

   auto fmt = mFFmpeg->GuessOutputFormat(selfmt->ToUTF8(),NULL,NULL);
   if (fmt == NULL)
   {
      //This shouldn't really happen
      mFormatName->SetLabel(wxString(_("Failed to guess format")));
      return;
   }
   mFormatName->SetLabel(wxString::Format(L"%s", *selfmtlong));

   AudacityAVCodecID selcdcid = AUDACITY_AV_CODEC_ID_NONE;

   Layout();
   Fit();
   return;
}

void ExportFFmpegOptions::DoOnCodecList()
{
   wxString *selcdc = nullptr;
   wxString* selcdclong = nullptr;

   FindSelectedCodec(&selcdc, &selcdclong);

   if (selcdc == nullptr)
   {
      return;
   }

   wxString* selfmt = nullptr;
   wxString* selfmtlong = nullptr;

   FindSelectedFormat(&selfmt, &selfmtlong);

   auto cdc = mFFmpeg->CreateEncoder(selcdc->ToUTF8());
   if (cdc == nullptr)
   {
      //This shouldn't really happen
      /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
      mCodecName->SetLabel(wxString(_("Failed to find the codec")));
      return;
   }

   mCodecName->SetLabel(wxString::Format(L"[%d] %s", (int) mFFmpeg->GetAudacityCodecID(cdc->GetId()).value, *selcdclong));

   if (selfmt != nullptr)
   {
      auto fmt = mFFmpeg->GuessOutputFormat(selfmt->ToUTF8(), nullptr, nullptr);
      if (fmt == nullptr)
      {
         selfmt = nullptr;
         selfmtlong = nullptr;
      }
   }

   int newselfmt = FetchCompatibleFormatList(
      mFFmpeg->GetAudacityCodecID(cdc->GetId()), selfmt);

   if (newselfmt >= 0)
      mFormatList->Select(newselfmt);

   Layout();
   Fit();
   return;
}

///
///
void ExportFFmpegOptions::OnFormatList(wxCommandEvent& WXUNUSED(event))
{
   DoOnFormatList();
}

///
///
void ExportFFmpegOptions::OnCodecList(wxCommandEvent& WXUNUSED(event))
{
   DoOnCodecList();
}


///
///
void ExportFFmpegOptions::OnOK()
{
   if( ReportIfBadCombination() )
      return;

   int selcdc = mCodecList->GetSelection();
   int selfmt = mFormatList->GetSelection();
   if (selcdc > -1)
      FFmpegCodec.Write( mCodecList->GetString( selcdc ) );
   if (selfmt > -1)
       FFmpegFormat.Write( mFormatList->GetString( selfmt ) );
   gPrefs->Flush();

   wxDialog::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   EndModal(wxID_OK);

   return;
}

void ExportFFmpegOptions::OnGetURL()
{
   HelpSystem::ShowHelp(this, L"Custom_FFmpeg_Export_Options");
}

StringSetting FFmpegCodec{ "/FileFormats/FFmpegCodec", L"" };
StringSetting FFmpegFormat{ "/FileFormats/FFmpegFormat", L"" };


#endif
