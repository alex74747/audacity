/**********************************************************************

Audacity: A Digital Audio Editor

CommonCommandFlags.h

Paul Licameli split from Menus.cpp

**********************************************************************/

#ifndef __AUDACITY_COMMON_COMMAND_FLAGS__
#define __AUDACITY_COMMON_COMMAND_FLAGS__

#include "Audacity.h"

#include "CommandFlag.h"

bool TracksSelectedPred( const AudacityProject &project );
bool AudioIOBusyPred( const AudacityProject &project );
bool TimeSelectedPred( const AudacityProject &project );
const CommandFlagOptions &cutCopyOptions();

AUDACITY_DLL_API const ReservedCommandFlag
   &AudioIONotBusyFlag(),
   &StereoRequiredFlag(),  //lda
   &NoiseReductionTimeSelectedFlag(),
   &TimeSelectedFlag(), // This is equivalent to check if there is a valid selection, so it's used for Zoom to Selection too
   &WaveTracksSelectedFlag(),
   &TracksExistFlag(),
   &TracksSelectedFlag(),
   &AnyTracksSelectedFlag(),
   &TrackPanelHasFocus();  //lll

AUDACITY_DLL_API const ReservedCommandFlag
   &AudioIOBusyFlag(), // lll
   &CaptureNotBusyFlag();

AUDACITY_DLL_API const ReservedCommandFlag
   &LabelTracksExistFlag(),
   &UnsavedChangesFlag(),
   &HasLastEffectFlag(),
   &UndoAvailableFlag(),
   &RedoAvailableFlag(),
   &ZoomInAvailableFlag(),
   &ZoomOutAvailableFlag(),
   &PlayRegionLockedFlag(),  //msmeyer
   &PlayRegionNotLockedFlag(),  //msmeyer
   &WaveTracksExistFlag(),
   &NoteTracksExistFlag(),  //gsw
   &NoteTracksSelectedFlag(),  //gsw
   &IsNotSyncLockedFlag(),  //awd
   &IsSyncLockedFlag(),  //awd
   &NotMinimizedFlag(), // prl
   &PausedFlag(), // jkc
   &PlayableTracksExistFlag(),
   &AudioTracksSelectedFlag(),
   &NoAutoSelect() // jkc
;

#endif
