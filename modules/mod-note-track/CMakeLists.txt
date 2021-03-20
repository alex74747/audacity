if( NOT TARGET portsmf )
   addlib( portsmf            portsmf     PORTSMF     NO    YES   "portSMF >= 0.1" )
endif()

set( SOURCES
   MIDIPlay.cpp
   MIDIPlay.h
   NoteTrack.cpp
   NoteTrack.h
)
set( LIBS
   mod-preference-pages-interface
   lib-audio-io-interface
   PortAudio::PortAudio
   portsmf
   PortMidi::PortMidi
   PRIVATE
   wxwidgets::wxwidgets
)
audacity_module( mod-note-track "${SOURCES}" "${LIBS}"
   "" "" )
