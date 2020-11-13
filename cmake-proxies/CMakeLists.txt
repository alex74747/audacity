# Allow user to globally set the library preference
cmd_option( ${_OPT}lib_preference
            "Library preference [system (if available), local]"
            "system"
            STRINGS "system" "local"
)

# Required libraries
#
#       directory          option      symbol      req   chk   version
addlib( wxWidgets          wxwidgets   WX          YES   NO    "" ) # must be first
addlib( expat              expat       EXPAT       YES   YES   "expat >= 2.1.0" )
addlib( libsoxr            soxr        SOXR        YES   YES   "soxr >= 0.1.1" )
addlib( portaudio-v19      portaudio   PORTAUDIO   YES   YES   "" )
addlib( sqlite             sqlite      SQLITE      YES   YES   "sqlite3 >= 3.32.0" )

# Optional libraries
#
#       directory          option      symbol      req   chk   version

addlib( portmixer          portmixer   PORTMIXER   NO    YES   "" )
if (NOT USE_PORTMIXER AND
   "EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT" IN_LIST
      EXPERIMENTAL_OPTIONS_LIST )
   message(FATAL_ERROR "EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT requires USE_PORTMIXER")
endif ()
