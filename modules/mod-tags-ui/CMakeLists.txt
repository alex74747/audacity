set( SOURCES
   TagsEditor.cpp
   TagsEditor.h
)
set( LIBRARIES
   Audacity
)
audacity_module( mod-tags-ui "${SOURCES}" "${LIBRARIES}"
   "" "" )
