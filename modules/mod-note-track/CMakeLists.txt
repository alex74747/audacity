if( NOT TARGET portsmf )
   addlib( portsmf            portsmf     PORTSMF     NO    YES   "portSMF >= 0.1" )
endif()

list( APPEND SOURCES
   MIDIPlay.cpp
   MIDIPlay.h
   NoteTrack.cpp
   NoteTrack.h
)
set( LIBS
   Audacity
   PortAudio::PortAudio
   portsmf
   PortMidi::PortMidi
)
audacity_module( mod-note-track "${SOURCES}" "${LIBS}"
   "" "" )
