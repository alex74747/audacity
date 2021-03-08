/**********************************************************************

Audacity: A Digital Audio Editor

ProjectManager.h

Paul Licameli split from AudacityProject.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_MANAGER__
#define __AUDACITY_PROJECT_MANAGER__

#include <memory>

#include "ClientData.h" // to inherit
#include "Registry.h"

class wxTimer;
class wxTimerEvent;
class wxWindow;

class AudacityProject;
struct AudioIOStartStreamOptions;

class ProjectWindow;

///\brief Object associated with a project for high-level management of the
/// project's lifetime, including creation, destruction, opening from file,
/// importing, pushing undo states, and reverting to saved states
class AUDACITY_DLL_API ProjectManager final
   : public wxEvtHandler
   , public ClientData::Base
{
   struct InsertedPanelItem;
   static void InitProjectWindow( ProjectWindow &window );

public:
   //! Type of function that adds panels to the main window
   using PanelFactory =
      std::function< wxWindow *(AudacityProject &, wxWindow * /*parent*/) >;

   //! To be statically constructed, it registers additional panels in the project window layout
   struct AUDACITY_DLL_API RegisteredPanel
      : public Registry::RegisteredItem<InsertedPanelItem>
   {
      RegisteredPanel( const Identifier &id,
         unsigned section, //!< 0 for top of layout, 1 for bottom
         PanelFactory factory,
         const Registry::Placement &placement = { wxEmptyString, {} } );

      struct AUDACITY_DLL_API Init{ Init(); };
   };

   static ProjectManager &Get( AudacityProject &project );
   static const ProjectManager &Get( const AudacityProject &project );

   explicit ProjectManager( AudacityProject &project );
   ProjectManager( const ProjectManager & ) PROHIBITED;
   ProjectManager &operator=( const ProjectManager & ) PROHIBITED;
   ~ProjectManager() override;

   // This is the factory for projects:
   static AudacityProject *New();

   // The function that imports files can act as a factory too, and for that
   // reason remains in this class, not in ProjectFileManager
   static void OpenFiles(AudacityProject *proj);

   // Return the given project if that is not NULL, else create a project.
   // Then open the given project path.
   // But if an exception escapes this function, create no NEW project.
   static AudacityProject *OpenProject(
      AudacityProject *pProject,
      const FilePath &fileNameArg, bool addtohistory = true);

   void ResetProjectToEmpty();

   static void SaveWindowSize();

   // Routine to estimate how many minutes of recording time are left on disk
   int GetEstimatedRecordingMinsLeftOnDisk(long lCaptureChannels = 0);
   // Converts number of minutes to human readable format
   TranslatableString GetHoursMinsString(int iMinutes);

   void SetStatusText( const TranslatableString &text, int number );
   void SetSkipSavePrompt(bool bSkip) { sbSkipPromptingForSave = bSkip; };

private:
   struct AUDACITY_DLL_API InsertedPanelItem final : Registry::SingleItem {
      static Registry::GroupItem &Registry();
   
      InsertedPanelItem(
         const Identifier &id, unsigned section, PanelFactory factory);

      unsigned mSection;
      PanelFactory mFactory;
   };

   void OnReconnectionFailure(wxCommandEvent & event);
   void OnCloseWindow(wxCloseEvent & event);
   void OnTimer(wxTimerEvent & event);
   void OnOpenAudioFile(wxCommandEvent & event);
   void OnStatusChange( wxCommandEvent& );

   void RestartTimer();

   // non-static data members
   AudacityProject &mProject;

   std::unique_ptr<wxTimer> mTimer;

   DECLARE_EVENT_TABLE()

   static bool sbWindowRectAlreadySaved;
   static bool sbSkipPromptingForSave;
};

// ID of a wxTimer event that ProjectManager emits
const int AudacityProjectTimerID = 5200;

// Guarantees registry exists before attempts to use it
static ProjectManager::RegisteredPanel::Init sInitRegisteredPanel;

#endif
