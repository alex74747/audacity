set(TARGET crashreporter)
set(TARGET_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

message( STATUS "========== Configuring ${TARGET} ==========" )

set(SOURCES 
	PRIVATE 
		CrashReportApp.h
		CrashReportApp.cpp
	)

add_executable(${TARGET})
target_sources(${TARGET} ${SOURCES})
target_link_libraries(${TARGET} breakpad_sender wxWidgets)

if(WIN32)
	set_target_properties(${TARGET} PROPERTIES WIN32_EXECUTABLE ON)
endif()

set_target_property_all( ${TARGET} RUNTIME_OUTPUT_DIRECTORY "${_EXEDIR}" )

organize_source( "${TARGET_ROOT}" "" "${SOURCES}" )