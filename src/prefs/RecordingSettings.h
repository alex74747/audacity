/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RecordingSettings.h
 
 Paul Licameli split from RecordingPrefs.h
 
 **********************************************************************/

#ifndef __AUDACITY_RECORDING_SETTINGS__
#define __AUDACITY_RECORDING_SETTINGS__

#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
   #define AILA_DEF_TARGET_PEAK 92
   #define AILA_DEF_DELTA_PEAK 2
   #define AILA_DEF_ANALYSIS_TIME 1000
   #define AILA_DEF_NUMBER_ANALYSIS 5
#endif

#endif
