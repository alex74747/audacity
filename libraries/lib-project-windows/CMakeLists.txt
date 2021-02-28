set( SOURCES
   ProjectWindowBase.cpp
   ProjectWindowBase.h
   ProjectWindows.cpp
   ProjectWindows.h
)
set( LIBRARIES
   lib-project-interface
   PRIVATE
   wxwidgets::wxwidgets
)
audacity_library( lib-project-windows "${SOURCES}" "${LIBRARIES}"
   "" ""
)
