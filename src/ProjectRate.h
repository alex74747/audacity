/**********************************************************************

Audacity: A Digital Audio Editor

ProjectRate.h

Paul Licameli split from ProjectSettings.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_RATE__
#define __AUDACITY_PROJECT_RATE__

class AudacityProject;

#include "Project.h"
#include <wx/event.h> // to declare custom event type

// Sent to the project when the rate changes
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
   EVT_PROJECT_RATE_CHANGE, wxCommandEvent);

///\brief Holds project sample rate
class AUDACITY_DLL_API ProjectRate final
   : public AttachedProjectObject
{
public:
   static ProjectRate &Get( AudacityProject &project );
   static const ProjectRate &Get( const AudacityProject &project );
   
   explicit ProjectRate(AudacityProject &project);
   ProjectRate( const ProjectRate & ) PROHIBITED;
   ProjectRate &operator=( const ProjectRate & ) PROHIBITED;

   void SetRate(double rate);
   double GetRate() const;

private:
   AudacityProject &mProject;
   double mRate;
};

#endif

