addlib( libmad             mad         LIBMAD      NO    YES   "mad >= 0.15.1b" )
addlib( lame               lame        LAME        YES   YES   "lame >= 3.100" )

#optional use of libid3tag
if ( NOT TARGET libid3tag )
   addlib( libid3tag          id3tag      LIBID3TAG   NO    YES   "id3tag >= 0.15.1b" )
endif()

list( APPEND SOURCES
   ExportMP3.cpp
   MP3Prefs.cpp
)
 
list( APPEND LIBS
   Audacity
)

if ( USE_LAME )
   list( APPEND LIBS lame )

   # Do not define these for Windows or Mac until further testing
   # can be done against ExportMP3.cpp.  If either are defined, the
   # build will fail.
   if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
      if( "${${_OPT}use_lame}" STREQUAL "local" )
         set( DISABLE_DYNAMIC_LOADING_LAME YES )
      endif()
   endif()

endif()

if ( USE_LIBMAD )

   list( APPEND SOURCES
      ImportMP3.cpp
   )

   list( APPEND LIBS
      libmad
   )

   if ( USE_LIBID3TAG )
      list( APPEND LIBS
         libid3tag
      )
   endif()

   if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
      list( APPEND MIMETYPES
         audio/mpeg
      )
      set( MIMETYPES "${MIMETYPES}" PARENT_SCOPE )
   endif()

endif()

audacity_module( mod-mp3 "${SOURCES}" "${LIBS}"
   "" "" )
