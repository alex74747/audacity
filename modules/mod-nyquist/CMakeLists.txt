set( SOURCES
   LoadNyquist.cpp
   LoadNyquist.h
   Nyquist.cpp
   Nyquist.h
)
set( LIBRARIES
   mod-effects-interface
   libnyquist
)
audacity_module( mod-nyquist "${SOURCES}" "${LIBRARIES}"
   "" "" )
