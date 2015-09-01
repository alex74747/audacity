#ifndef __AUDACITY_TRANSPORT_MENU_COMMANDS__
#define __AUDACITY_TRANSPORT_MENU_COMMANDS__

#include "../Experimental.h"

class AudacityProject;
class CommandManager;

class TransportMenuCommands
{
   TransportMenuCommands(const TransportMenuCommands&);
   TransportMenuCommands& operator= (const TransportMenuCommands&);
public:
   TransportMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);
   void CreateNonMenuCommands(CommandManager *c);

   void OnPlayStop();
   bool DoPlayStopSelect(bool click, bool shift);

private:
   void OnPlayStopSelect();
   void OnPlayLooped();
public:
   void OnPause();
private:
   void OnSkipStart();
   void OnSkipEnd();
public:
   void OnRecord();
private:
   void OnTimerRecord();
   void OnRecordAppend();
public:
   void OnTogglePinnedHead();
private:
   void OnTogglePlayRecording();
   void OnToggleSWPlaythrough();
   void OnToggleSoundActivated();
   void OnSoundActivated();
#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
   void OnToggleAutomatedInputLevelAdjustment();
#endif
   void OnRescanDevices();

   // non-menu commands
public:
   void OnStop();
private:
   void OnPlayOneSecond();
   void OnPlayToSelection();
   void OnPlayBeforeSelectionStart();
   void OnPlayAfterSelectionStart();
   void OnPlayBeforeSelectionEnd();
   void OnPlayAfterSelectionEnd();

   AudacityProject *const mProject;
};

#endif
