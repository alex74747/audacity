set( SOURCES
   CommandContext.cpp
   CommandContext.h
   CommandFlag.cpp
   CommandFlag.h
   CommandFunctors.h
   CommandID.h
   CommandManager.cpp
   CommandManager.h
   CommandTargets.cpp
   CommandTargets.h
   Journal.cpp
   Journal.h
   JournalEvents.cpp
   JournalEvents.h
   JournalOutput.cpp
   JournalOutput.h
   JournalRegistry.cpp
   JournalRegistry.h
   JournalWindowPaths.cpp
   JournalWindowPaths.h
   Menus.cpp
   Menus.h
)
set( LIBRARIES
   lib-menus-interface
   lib-project-windows-interface
   lib-widgets-interface
   lib-xml-interface
   PRIVATE
   wxwidgets::wxwidgets
)
audacity_library( lib-command-manager "${SOURCES}" "${LIBRARIES}"
   "" ""
)
