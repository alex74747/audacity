#[[
Toolkit-neutral abstractions of menus, menu items, menu toolbars, and iterators
over them.
]]#

set( SOURCES
   BasicMenu.cpp
   BasicMenu.h
   Keyboard.cpp
   Keyboard.h
   MenuHandle.cpp
   MenuHandle.h
   wxWidgetsWindowPlacement.cpp
   wxWidgetsWindowPlacement.h
)
set( LIBRARIES
   lib-basic-ui-interface
   lib-utility-interface
   PRIVATE
   wxBase
)
audacity_library( lib-menus "${SOURCES}" "${LIBRARIES}"
   "" ""
)
