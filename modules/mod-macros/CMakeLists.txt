set( SOURCES
   BatchCommandDialog.cpp
   BatchCommandDialog.h
   BatchCommands.cpp
   BatchCommands.h
   BatchEvalCommand.cpp
   BatchEvalCommand.h
   BatchPrefs.cpp
   BatchPrefs.h
   BatchProcessDialog.cpp
   BatchProcessDialog.h
)
set( LIBRARIES
   Audacity
)
audacity_module( mod-macros "${SOURCES}" "${LIBRARIES}"
   "" "" )
