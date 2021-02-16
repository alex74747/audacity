addlib( libsndfile         sndfile     SNDFILE     YES   YES   "sndfile >= 1.0.28" )

#optional use of libid3tag
if ( NOT TARGET libid3tag )
   addlib( libid3tag          id3tag      LIBID3TAG   NO    YES   "id3tag >= 0.15.1b" )
endif()

list( APPEND SOURCES
   DropTarget.cpp
   Export.cpp
   Export.h
   ExportCL.cpp
   ExportMultiple.cpp
   ExportMultiple.h
   ExtImportPrefs.cpp
   ExtImportPrefs.h
   FileFormats.cpp
   FileFormats.h
   FormatClassifier.cpp
   FormatClassifier.h
   Import.cpp
   Import.h
   ImportAUP.cpp
   ImportForwards.h
   ImportLOF.cpp
   ImportPlugin.cpp
   ImportPlugin.h
   ImportRaw.cpp
   ImportRaw.h
   MultiFormatReader.cpp
   MultiFormatReader.h
   RawAudioGuess.cpp
   RawAudioGuess.h
   SpecPowerMeter.cpp
   SpecPowerMeter.h
)
audacity_module( mod-import-export "${SOURCES}"
   "Audacity;libsndfile;libid3tag"
   "" "" )

if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
   # Build the list of mimetypes
   #
   # (Don't use generator expressions since it will leave null/empty
   # entries in the list.)
   if( USE_SNDFILE )
      list( APPEND MIMETYPES
         audio/basic
         audio/x-aiff
         audio/x-wav
      )
      set( MIMETYPES "${MIMETYPES}" PARENT_SCOPE )
   endif()
endif()
