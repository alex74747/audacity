if ( USE_LIBFLAC )

   set( SOURCES
      ExportFLAC.cpp
      ImportFLAC.cpp
   )

   set( LIBRARIES
      mod-import-export-interface
      FLAC::FLAC
      FLAC::FLAC++
   )

   if ( USE_LIBID3TAG )
      list( APPEND LIBS
         libid3tag::libid3tag
      )
   endif()

   audacity_module( mod-flac "${SOURCES}" "${LIBRARIES}"
      "" "" )

   if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
      list( APPEND MIMETYPES
         audio/flac
         audio/x-flac
      )
      set( MIMETYPES "${MIMETYPES}" PARENT_SCOPE )
   endif()

endif()
