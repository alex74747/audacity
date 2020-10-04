/**********************************************************************

Audacity: A Digital Audio Editor

ProjectSettings.h

Paul Licameli split from AudacityProject.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_SETTINGS__
#define __AUDACITY_PROJECT_SETTINGS__

#include <atomic>

#include "Observer.h"
#include "Project.h"
#include "Prefs.h" // to inherit
#include "audacity/Types.h"

class AudacityProject;

enum
{
   SNAP_OFF,
   SNAP_NEAREST,
   SNAP_PRIOR
};

namespace ToolCodes {
enum {
   // The buttons that are in the Tools toolbar must be in correspondence
   // with the first few
   selectTool,
   envelopeTool,
   drawTool,
   zoomTool,
   multiTool,

#ifdef EXPERIMENTAL_BRUSH_TOOL
   brushTool,
#endif

   numTools,
   firstTool = selectTool,
};
}

struct ProjectSettingsEvent {
   enum Type : int {
      ChangedTool,
      ChangedSnapTo,
      ChangedSelectionFormat,
      ChangedAudioTimeFormat,
      ChangedFrequencyFormat,
      ChangedBandwidthFormat,
   } type;
   int oldValue{ -1 };
   int newValue{ -1 };
};

///\brief Holds various per-project settings values,
/// and sends events to the project when certain values change
class AUDACITY_DLL_API ProjectSettings final
   : public AttachedProjectObject
   , public Observer::Publisher<ProjectSettingsEvent>
   , private PrefsListener
{
public:
   static ProjectSettings &Get( AudacityProject &project );
   static const ProjectSettings &Get( const AudacityProject &project );
   

   explicit ProjectSettings( AudacityProject &project );
   ProjectSettings( const ProjectSettings & ) PROHIBITED;
   ProjectSettings &operator=( const ProjectSettings & ) PROHIBITED;


   bool GetTracksFitVerticallyZoomed() const { return mTracksFitVerticallyZoomed; } //lda
   void SetTracksFitVerticallyZoomed(bool flag) { mTracksFitVerticallyZoomed = flag; } //lda

   bool GetShowId3Dialog() const { return mShowId3Dialog; } //lda
   void SetShowId3Dialog(bool flag) { mShowId3Dialog = flag; } //lda

   // Snap To

   void SetSnapTo(int snap);
   int GetSnapTo() const;

   // Current tool

   void SetTool(int tool);
   int GetTool() const { return mCurrentTool; }

   // Current brush radius
   void SetBrushRadius(int brushRadius) { mCurrentBrushRadius = brushRadius; }
   int GetBrushRadius() const { return mCurrentBrushRadius; }

   void SetSmartSelection(bool isSelected) { mbSmartSelection = isSelected; }
   bool IsSmartSelection() const { return mbSmartSelection; }

   void SetOvertones(bool isSelected) { mbOvertones = isSelected; }
   bool IsOvertones() const { return mbOvertones; }
   
   // Selection Format
   void SetSelectionFormat(const NumericFormatSymbol & format);
   const NumericFormatSymbol & GetSelectionFormat() const;

   // AudioTime format
   void SetAudioTimeFormat(const NumericFormatSymbol & format);
   const NumericFormatSymbol & GetAudioTimeFormat() const;

   // Spectral Selection Formats
   void SetFrequencySelectionFormatName(const NumericFormatSymbol & format);
   const NumericFormatSymbol & GetFrequencySelectionFormatName() const;

   void SetBandwidthSelectionFormatName(const NumericFormatSymbol & format);
   const NumericFormatSymbol & GetBandwidthSelectionFormatName() const;

   bool IsSoloSimple() const { return mSoloPref == wxT("Simple"); }
   bool IsSoloNone() const { return mSoloPref == wxT("None"); }

   bool EmptyCanBeDirty() const { return mEmptyCanBeDirty; }

   bool GetShowSplashScreen() const { return mShowSplashScreen; }

private:
   void UpdatePrefs() override;

   AudacityProject &mProject;

   NumericFormatSymbol mSelectionFormat;
   NumericFormatSymbol mFrequencySelectionFormatName;
   NumericFormatSymbol mBandwidthSelectionFormatName;
   NumericFormatSymbol mAudioTimeFormat;

   wxString mSoloPref;

   int mSnapTo;

   int mCurrentTool;
   int mCurrentBrushRadius;
   int mCurrentBrushHop;
   bool mbSmartSelection { false };
   bool mbOvertones { false };
   
   bool mTracksFitVerticallyZoomed{ false };  //lda
   bool mShowId3Dialog{ true }; //lda
   bool mIsSyncLocked{ false };
   bool mEmptyCanBeDirty;
   bool mShowSplashScreen;
};

#endif
