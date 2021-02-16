addlib( libflac            flac        LIBFLAC     NO    YES   "flac >= 1.3.1" "flac++ >= 1.3.1" )

#optional use of libid3tag
if ( NOT TARGET libid3tag )
   addlib( libid3tag          id3tag      LIBID3TAG   NO    YES   "id3tag >= 0.15.1b" )
endif()

if ( USE_LIBFLAC )

   list( APPEND SOURCES
      ExportFLAC.cpp
      ImportFLAC.cpp
   )

   list( APPEND LIBS
      mod-import-export-interface
      libflac
   )

   if ( USE_LIBID3TAG )
      list( APPEND LIBS
         libid3tag
      )
   endif()

   audacity_module( mod-flac "${SOURCES}" "${LIBS}"
      "" "" )

   if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
      list( APPEND MIMETYPES
         audio/flac
         audio/x-flac
      )
      set( MIMETYPES "${MIMETYPES}" PARENT_SCOPE )
   endif()

endif()
