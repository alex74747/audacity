/**********************************************************************

  Audacity: A Digital Audio Editor

  AudioIOListener.h

  Dominic Mazzoni

  Use the PortAudio library to play and record sound

**********************************************************************/

#ifndef __AUDACITY_AUDIO_IO_LISTENER__
#define __AUDACITY_AUDIO_IO_LISTENER__

#include "Audacity.h"

class AutoSaveFile;

class AUDACITY_DLL_API AudioIOListener /* not final */ {
public:
   AudioIOListener() {}
   virtual ~AudioIOListener() {}

   // Runs on the main thread
   // Pass 0 when audio stops, positive when it starts:
   virtual void OnAudioIORate(int rate) = 0;

   // Runs on the main thread
   virtual void OnAudioIOStartRecording() = 0;

   // Runs on the main thread
   virtual void OnAudioIOStopRecording() = 0;

   // Runs on the audio buffering thread
   virtual void OnAudioIONewBlockFiles(const AutoSaveFile & blockFileLog) = 0;

   // Commit the addition of temporary recording tracks into the project
   // Runs on the main thread
   virtual void OnCommitRecording() = 0;

   // During recording, the threshold for sound activation has been crossed
   // in either direction
   // Runs on the audio callback thread
   virtual void OnSoundActivationThreshold() = 0;

};

#endif
