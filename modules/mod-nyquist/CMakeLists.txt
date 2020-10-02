set( SOURCES
   LoadNyquist.cpp
   LoadNyquist.h
   Nyquist.cpp
   Nyquist.h
)
set( LIBRARIES
   Audacity
   libnyquist
)
audacity_module( mod-nyquist "${SOURCES}" "${LIBRARIES}"
   "" "" )
