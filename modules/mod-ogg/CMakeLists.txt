if ( USE_LIBFLAC AND USE_LIBVORBIS )

   set( SOURCES
      ExportOGG.cpp
      ImportOGG.cpp
   )

   set( LIBS
      mod-import-export-interface
      $<$<BOOL:${USE_LIBOGG}>:Ogg::ogg>
      Vorbis::vorbis
      Vorbis::vorbisfile
      Vorbis::vorbisenc
      $<$<BOOL:${USE_LIBOPUS}>:Opus::opus>
   )

   audacity_module( mod-ogg "${SOURCES}" "${LIBS}"
      "" "" )

   if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
      list( APPEND MAC_RESOURCES
         "${CMAKE_CURRENT_SOURCE_DIR}/mac/resources/AudacityOGG.icns"
      )
   elseif( NOT CMAKE_SYSTEM_NAME MATCHES "Windows" )
      list( APPEND MIMETYPES
         application/ogg
         audio/x-vorbis+ogg
      )
   endif()
  
   foreach(var MIMETYPES MAC_RESOURCES)
      set( ${var} "${${var}}" PARENT_SCOPE )
   endforeach()

endif()
