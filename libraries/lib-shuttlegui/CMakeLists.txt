set( SOURCES
   ReadOnlyText.h
   Shuttle.cpp
   Shuttle.h
   ShuttleGui.cpp
   ShuttleGui.h
   ShuttlePrefs.cpp
   ShuttlePrefs.h
   WrappedType.cpp
   WrappedType.h
   wxTextCtrlWrapper.h
)
set( LIBRARIES
   lib-theme-interface
   lib-wx-wrappers-interface
   PRIVATE
   wxwidgets::wxwidgets
)
audacity_library( lib-shuttlegui "${SOURCES}" "${LIBRARIES}"
   "" ""
)
