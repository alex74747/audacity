set( SOURCES
   PrefsDialog.cpp
   PrefsDialog.h
   PrefsPanel.cpp
   PrefsPanel.h
)
set( LIBRARIES
   lib-project-windows-interface
   lib-help-system-interface
   PRIVATE
   wxwidgets::wxwidgets
)
audacity_library( lib-preferences-dialog "${SOURCES}" "${LIBRARIES}"
   "" ""
)
