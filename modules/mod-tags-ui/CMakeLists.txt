set( SOURCES
   TagsEditor.cpp
   TagsEditor.h
)
set( LIBRARIES
   mod-tags-interface
   Audacity
)
audacity_module( mod-tags-ui "${SOURCES}" "${LIBRARIES}"
   "" "" )
