addlib( twolame            twolame     LIBTWOLAME  NO    YES   "twolame >= 0.3.13" )

#optional use of libid3tag
if ( NOT TARGET libid3tag )
   addlib( libid3tag          id3tag      LIBID3TAG   NO    YES   "id3tag >= 0.15.1b" )
endif()

if ( USE_LIBTWOLAME )

   list( APPEND SOURCES
      ExportMP2.cpp
   )

   list( APPEND LIBS
      Audacity
      twolame
   )

   if ( USE_LIBID3TAG )
      list( APPEND LIBS
         libid3tag
      )
   endif()

   audacity_module( mod-mp2 "${SOURCES}" "${LIBS}"
      "" "" )

endif()
