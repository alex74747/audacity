set( SOURCES
   TimerRecordDialog.cpp
   TimerRecordDialog.h
)
set( LIBRARIES
   mod-import-export-interface
)
audacity_module( mod-timer-record "${SOURCES}" "${LIBRARIES}"
   "" "" )
