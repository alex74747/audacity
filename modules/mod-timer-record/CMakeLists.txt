set( SOURCES
   TimerRecordDialog.cpp
   TimerRecordDialog.h
)
set( LIBRARIES
   Audacity
)
audacity_module( mod-timer-record "${SOURCES}" "${LIBRARIES}"
   "" "" )
