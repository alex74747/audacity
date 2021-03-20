addlib( sqlite             sqlite      SQLITE      YES   YES   "sqlite3 >= 3.31.1" )

set( SOURCES
   ActiveProjects.cpp
   ActiveProjects.h
   DBConnection.cpp
   DBConnection.h
   ProjectFileIO.cpp
   ProjectFileIO.h
   ProjectSerializer.cpp
   ProjectSerializer.h
   SqliteSampleBlock.cpp
)
set( LIBRARIES
   lib-string-utils-interface
   lib-wave-track-interface
   lib-project-history-interface
   lib-sentry-reporting
   sqlite
   PRIVATE
   wxBase
)
audacity_library( lib-project-file-io "${SOURCES}" "${LIBRARIES}"
   ""
   ""
)
