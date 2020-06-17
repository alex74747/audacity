do_directory(
   INCLUDE_PRIVATE
      src/command-classes
      src/commands
      src/effects
      src/etc
      src/export
      src/import
      src/ondemand
      src/prefs
      src/toolbars
      src/track

   PRIVATE
      src/utility/math
      src/widgets/CellularPanel
      src/widgets/DialogDefinition
      src/widgets/etc
      src/widgets/help
      src/widgets/theme
      src/widgets/utility
      src/xml
      wxWidgets

   SOURCES
      AboutDialog.cpp
      AboutDialog.h
      BatchCommandDialog.cpp
      BatchCommandDialog.h
      BatchProcessDialog.cpp
      BatchProcessDialog.h
      Benchmark.cpp
      Benchmark.h
      ScoreAlignDialog.cpp
      ScoreAlignDialog.h
      Screenshot.cpp
      Screenshot.h
      SoundActivatedRecord.cpp
      SoundActivatedRecord.h
      SplashDialog.cpp
      SplashDialog.h
      TimeDialog.cpp
      TimeDialog.h
      TimerRecordDialog.cpp
      TimerRecordDialog.h
)

# For AboutDialog.h
# Add a target that will provide the git revision info
# whenever it changes.  (Must be done at build time, not
# configuration time.)
def_vars()
if( GIT_FOUND )
   add_custom_target(
      version
      COMMAND
         ${CMAKE_COMMAND} -D GIT=${GIT_EXECUTABLE}
                          -D _PRVDIR=${_PRVDIR}
                          -P ${CMAKE_MODULE_PATH}/Version.cmake
      WORKING_DIRECTORY
         ${topdir}
   )
   add_dependencies( dialogs version )
endif()
