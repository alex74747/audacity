set( SOURCES
   LoadNyquist.cpp
   LoadNyquist.h
   Nyquist.cpp
   Nyquist.h
)
set( LIBRARIES
   mod-effects-interface
   libnyquist
   mod-spectrum-ui-interface
)
audacity_module( mod-nyquist "${SOURCES}" "${LIBRARIES}"
   "" "" )
