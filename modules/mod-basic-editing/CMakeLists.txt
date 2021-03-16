list( APPEND SOURCES
   EditUtilities.cpp
   EditUtilities.h
)
audacity_module( mod-basic-editing "${SOURCES}"
   "mod-track-panel-interface;Audacity"
   "" "" )
