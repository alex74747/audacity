set( SOURCES
   SelectMenus.cpp
)
set( LIBRARIES
   mod-control-tool-bar-interface
   mod-label-track-ui-interface
   mod-wave-track-ui-interface
)
audacity_module( mod-select-menus "${SOURCES}" "${LIBRARIES}"
   "" "" )
