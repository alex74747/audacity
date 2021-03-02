set( SOURCES
   MixAndRender.cpp
   MixAndRender.h
   SampleBlock.cpp
   SampleBlock.h
   Sequence.cpp
   Sequence.h
   WaveClip.cpp
   WaveClip.h
   WaveTrack.cpp
   WaveTrack.h
)
set( LIBRARIES
   lib-sample-track-interface
   lib-project-rate-interface
   PRIVATE
   wxBase
)
audacity_library( lib-wave-track "${SOURCES}" "${LIBRARIES}"
   "" ""
)
