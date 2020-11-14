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
   mod-import-export-interface
   mod-effects-interface
   mod-scripting-interface
)
audacity_module( mod-macros "${SOURCES}" "${LIBRARIES}"
   "" "" )
