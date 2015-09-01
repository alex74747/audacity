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
private:
   void OnPlayStopSelect();
   void OnPlayLooped();
   void OnPause();
   void OnSkipStart();
   void OnSkipEnd();
public:
   void OnRecord();
private:
   void OnTimerRecord();
   void OnRecordAppend();
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
   void OnPlayBeforeAndAfterSelectionStart();
   void OnPlayBeforeAndAfterSelectionEnd();
   void OnPlayCutPreview();
   bool MakeReadyToPlay(bool loop = false, bool cutpreview = false); // Helper function that sets button states etc.

   void OnInputDevice();
   void OnOutputDevice();
   void OnAudioHost();
   void OnInputChannels();
   void OnOutputGain();
   void OnOutputGainInc();
   void OnOutputGainDec();
   void OnInputGain();
   void OnInputGainInc();
   void OnInputGainDec();
   void OnPlayAtSpeed();
   void OnPlayAtSpeedLooped();
   void OnPlayAtSpeedCutPreview();
   void OnSetPlaySpeed();

   AudacityProject *const mProject;
};

#endif
