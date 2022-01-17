/**********************************************************************

Audacity: A Digital Audio Editor

ProjectSettings.cpp

Paul Licameli split from AudacityProject.cpp

**********************************************************************/

#include "ProjectSettings.h"



#include "AudioIOBase.h"
#include "Project.h"
#include "QualitySettings.h"
#include "NumericTextCtrl.h"
#include "XMLWriter.h"
#include "XMLTagHandler.h"

wxDEFINE_EVENT(EVT_PROJECT_SETTINGS_CHANGE, wxCommandEvent);

static const AudacityProject::AttachedObjects::RegisteredFactory
sProjectSettingsKey{
  []( AudacityProject &project ){
     auto result = std::make_shared< ProjectSettings >( project );
     return result;
   }
};

ProjectSettings &ProjectSettings::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< ProjectSettings >(
      sProjectSettingsKey );
}

const ProjectSettings &ProjectSettings::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

ProjectSettings::ProjectSettings(AudacityProject &project)
   : mProject{ project }
   , mSelectionFormat{ NumericTextCtrl::LookupFormat(
      NumericConverter::TIME,
      gPrefs->Read(wxT("/SelectionFormat"), wxT("")))
}
, mAudioTimeFormat{ NumericTextCtrl::LookupFormat(
   NumericConverter::TIME,
   gPrefs->Read(wxT("/AudioTimeFormat"), wxT("hh:mm:ss")))
}
, mFrequencySelectionFormatName{ NumericTextCtrl::LookupFormat(
   NumericConverter::FREQUENCY,
   gPrefs->Read(wxT("/FrequencySelectionFormatName"), wxT("")) )
}
, mBandwidthSelectionFormatName{ NumericTextCtrl::LookupFormat(
   NumericConverter::BANDWIDTH,
   gPrefs->Read(wxT("/BandwidthSelectionFormatName"), wxT("")) )
}
, mSnapTo( gPrefs->Read(wxT("/SnapTo"), SNAP_OFF) )
, mCurrentBrushRadius ( 5 )
{
   bool multiToolActive = false;
   gPrefs->Read(wxT("/GUI/ToolBars/Tools/MultiToolActive"), &multiToolActive);

   if (multiToolActive)
      mCurrentTool = ToolCodes::multiTool;
   else
      mCurrentTool = ToolCodes::selectTool;

   UpdatePrefs();
}

void ProjectSettings::UpdatePrefs()
{
   gPrefs->Read(wxT("/AudioFiles/ShowId3Dialog"), &mShowId3Dialog, true);
   gPrefs->Read(wxT("/GUI/EmptyCanBeDirty"), &mEmptyCanBeDirty, true);
   gPrefs->Read(wxT("/GUI/ShowSplashScreen"), &mShowSplashScreen, true);
   mSoloPref = TracksBehaviorsSolo.Read();
   // Update the old default to the NEW default.
   if (mSoloPref == wxT("Standard"))
      mSoloPref = wxT("Simple");
   gPrefs->Read(wxT("/GUI/TracksFitVerticallyZoomed"),
      &mTracksFitVerticallyZoomed, false);
   //   gPrefs->Read(wxT("/GUI/UpdateSpectrogram"),
   //     &mViewInfo.bUpdateSpectrogram, true);

   // This code to change an empty projects rate is currently disabled, after
   // discussion.  The rule 'Default sample rate' only affects newly created
   // projects was felt to be simpler and better.
#if 0
   // The DefaultProjectSample rate is the rate for new projects.
   // Do not change this project's rate, unless there are no tracks.
   if( TrackList::Get( *this ).size() == 0){
      mRate = QualityDefaultSampleRate.Read();
      // If necessary, we change this rate in the selection toolbar too.
      auto bar = SelectionBar::Get( *this );
      bar.SetRate( mRate );
   }
#endif
}

const NumericFormatSymbol &
ProjectSettings::GetFrequencySelectionFormatName() const
{
   return mFrequencySelectionFormatName;
}

void ProjectSettings::SetFrequencySelectionFormatName(
   const NumericFormatSymbol & formatName)
{
   if (formatName != mFrequencySelectionFormatName) {
      mFrequencySelectionFormatName = formatName;
      Publish({ ProjectSettingsEvent::ChangedFrequencyFormat });
   }
}

const NumericFormatSymbol &
ProjectSettings::GetBandwidthSelectionFormatName() const
{
   return mBandwidthSelectionFormatName;
}

void ProjectSettings::SetBandwidthSelectionFormatName(
   const NumericFormatSymbol & formatName)
{
   if (formatName != mBandwidthSelectionFormatName) {
      mBandwidthSelectionFormatName = formatName;
      Publish({ ProjectSettingsEvent::ChangedBandwidthFormat });
   }
}

void ProjectSettings::SetSelectionFormat(const NumericFormatSymbol & format)
{
   if (format != mSelectionFormat) {
      mSelectionFormat = format;
      Publish({ ProjectSettingsEvent::ChangedSelectionFormat });
   }
}

const NumericFormatSymbol & ProjectSettings::GetSelectionFormat() const
{
   return mSelectionFormat;
}

void ProjectSettings::SetAudioTimeFormat(const NumericFormatSymbol & format)
{
   if (format != mAudioTimeFormat) {
      mAudioTimeFormat = format;
      Publish({ ProjectSettingsEvent::ChangedAudioTimeFormat });
   }
}

const NumericFormatSymbol & ProjectSettings::GetAudioTimeFormat() const
{
   return mAudioTimeFormat;
}

void ProjectSettings::SetSnapTo(int snap)
{
   if (snap != mSnapTo) {
      mSnapTo = snap;
      Publish({ ProjectSettingsEvent::ChangedSnapTo });
   }
}

int ProjectSettings::GetSnapTo() const
{
   return mSnapTo;
}

// Move it to source file, to trigger event
void ProjectSettings::SetTool(int tool) {
   mCurrentTool = tool;
   Publish({ ProjectSettingsEvent::ChangedTool });
}

static ProjectFileIORegistry::AttributeWriterEntry entry {
[](const AudacityProject &project, XMLWriter &xmlFile){
   auto &settings = ProjectSettings::Get(project);
   xmlFile.WriteAttr(wxT("snapto"), settings.GetSnapTo() ? wxT("on") : wxT("off"));
   xmlFile.WriteAttr(wxT("selectionformat"),
                     settings.GetSelectionFormat().Internal());
   xmlFile.WriteAttr(wxT("frequencyformat"),
                     settings.GetFrequencySelectionFormatName().Internal());
   xmlFile.WriteAttr(wxT("bandwidthformat"),
                     settings.GetBandwidthSelectionFormatName().Internal());
}
};

static ProjectFileIORegistry::AttributeReaderEntries entries {
// Just a pointer to function, but needing overload resolution as non-const:
(ProjectSettings& (*)(AudacityProject &)) &ProjectSettings::Get, {
   // PRL:  The following have persisted as per-project settings for long.
   // Maybe that should be abandoned.  Enough to save changes in the user
   // preference file.
   { "snapto", [](auto &settings, auto value){
      settings.SetSnapTo(value.ToWString() == wxT("on") ? true : false);
   } },
   { "selectionformat", [](auto &settings, auto value){
      settings.SetSelectionFormat(NumericConverter::LookupFormat(
              NumericConverter::TIME, value.ToWString()));
   } },
   { "frequencyformat", [](auto &settings, auto value){
      settings.SetFrequencySelectionFormatName(
              NumericConverter::LookupFormat(
                 NumericConverter::FREQUENCY, value.ToWString()));
   } },
   { "bandwidthformat", [](auto &settings, auto value){
      settings.SetBandwidthSelectionFormatName(
              NumericConverter::LookupFormat(
                 NumericConverter::BANDWIDTH, value.ToWString()));
   } },
} };

ChoiceSetting ProjectSettings::TracksBehaviorsSolo{
   wxT("/GUI/Solo"),
   {
      ByColumns,
      { XO("Simple"),  XO("Multi-track"), XO("None") },
      { wxT("Simple"), wxT("Multi"),      wxT("None") }
   },
   0, // "Simple"
};
