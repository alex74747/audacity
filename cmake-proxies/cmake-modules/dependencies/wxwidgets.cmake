function( ensure_wxBase )
   if( NOT TARGET wxBase )
      set( target wxwidgets::wxwidgets )
      add_library( wxBase INTERFACE )
      set( PROPERTIES
         INTERFACE_INCLUDE_DIRECTORIES
         INTERFACE_COMPILE_DEFINITIONS
         INTERFACE_COMPILE_OPTIONS
         INTERFACE_LINK_DIRECTORIES
         INTERFACE_LINK_LIBRARIES
      )
      foreach( property ${PROPERTIES} )
         get_target_property( value "${target}" "${property}" )
         if( value )
            set_target_properties( wxBase PROPERTIES "${property}" "${value}" )
	 endif()
      endforeach()
      # wxBase exposes only the GUI-less subset of full wxWidgets
      # Also prohibit use of some other headers by pre-defining their include guards
      # wxUSE_GUI=0 doesn't exclude all of wxCore dependency, and the application
      # object and event loops are in wxBase, but we want to exclude their use too
      target_compile_definitions( wxBase INTERFACE
         "wxUSE_GUI=0"
       
         # Don't use app.h
         _WX_APP_H_BASE_
      
         # Don't use evtloop.h
         _WX_EVTLOOP_H_
      
         # Don't use image.h
         _WX_IMAGE_H
      
         # Don't use colour.h
         _WX_COLOUR_H_BASE_
      
         # Don't use brush.h
         _WX_BRUSH_H_BASE_
       
        # Don't use pen.h
        _WX_PEN_H_BASE_
      )
   endif()
endfunction()

if( ${_OPT}use_wxwidgets STREQUAL "system" ) 
    if( NOT TARGET wxwidgets::wxwidgets )
        add_library( wxwidgets::wxwidgets INTERFACE IMPORTED GLOBAL)
    endif()

    foreach( lib base core html xml xrc qa aui adv )
       set( target "wxwidgets::${lib}" )
       if( NOT TARGET ${target} )
          add_library( ${target} ALIAS wxwidgets::wxwidgets )
       endif()
    endforeach()

    if( NOT TARGET wxwidgets::wxwidgets )
        add_library( wxwidgets::wxwidgets ALIAS wxwidgets::wxwidgets )
    endif()

    if( wxWidgets_INCLUDE_DIRS_NO_SYSTEM )
        target_include_directories( wxwidgets::wxwidgets INTERFACE ${wxWidgets_INCLUDE_DIRS_NO_SYSTEM} )
    else()
        target_include_directories( wxwidgets::wxwidgets INTERFACE ${wxWidgets_INCLUDE_DIRS} )
    endif() 

    target_compile_definitions( wxwidgets::wxwidgets INTERFACE
        ${wxWidgets_DEFINITIONS_GENERAL}
        $<$<CONFIG:Debug>:
            ${wxWidgets_DEFINITIONS_DEBUG}
        >
        $<$<NOT:$<CONFIG:Debug>>:
            ${wxWidgets_DEFINITIONS_OPTIMIZED}
        >
    )

    target_link_directories( wxwidgets::wxwidgets INTERFACE
        $<$<PLATFORM_ID:Windows>:
           ${wxWidgets_LIB_DIR}
        >
    )

    target_link_libraries( wxwidgets::wxwidgets INTERFACE
        ${wxWidgets_LIBRARIES}
        $<$<NOT:$<PLATFORM_ID:Windows>>:
           z
        >
    )

    set( toolkit "${wxWidgets_LIBRARIES}" )

    if( "${toolkit}" MATCHES ".*gtk2.*" )
        set( gtk gtk+-2.0 )
        set( glib glib-2.0 )
    elseif( "${toolkit}" MATCHES ".*gtk3.*" )
        set( gtk gtk+-3.0 )
        set( glib glib-2.0 )
    elseif( "${toolkit}" MATCHES ".*gtk4.*" )
        set( gtk gtk+-4.0 )
        set( glib glib-2.0 )
    endif()

    ensure_wxBase()
else()
    set_target_properties(wxwidgets::base PROPERTIES IMPORTED_GLOBAL On)
    ensure_wxBase()
endif()

if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )

    if( NOT DEFINED gtk )
        set( gtk gtk+-2.0 )
        set( glib glib-2.0 )
    endif()

    pkg_check_modules( GTK REQUIRED IMPORTED_TARGET GLOBAL ${gtk} )
    pkg_check_modules( GLIB REQUIRED IMPORTED_TARGET GLOBAL ${glib} )
endif()

