#optional use of libid3tag
if ( NOT TARGET libid3tag )
   addlib( libid3tag          id3tag      LIBID3TAG   NO    YES   "id3tag >= 0.15.1b" )
endif()

list( APPEND SOURCES
   ExportPCM.cpp
   ImportPCM.cpp
)
list( APPEND LIBRARIES
   mod-import-export-interface
   libsndfile
)
if (USE_LIBID3TAG)
   list( APPEND LIBRARIES libid3tag )
endif()

audacity_module( mod-pcm "${SOURCES}" "${LIBRARIES}"
   "" "" )
