include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_subdirectory(01-internal)

add_subdirectory(02-fullserver)

add_subdirectory(03-methods)

add_subdirectory(04-prerecorded)

add_subdirectory(05-sessions)

#add_subdirectory(06-handlers)

add_subdirectory(07-tools)

ADD_CUSTOM_TARGET(auto.sh ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/auto.sh
	COMMAND test "${CMAKE_CURRENT_SOURCE_DIR}" = "${CMAKE_CURRENT_BINARY_DIR}" || cp ${CMAKE_CURRENT_SOURCE_DIR}/auto.sh  ${CMAKE_CURRENT_BINARY_DIR}/auto.sh
	)

ADD_CUSTOM_TARGET(onion.ini ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/onion.ini
	COMMAND test "${CMAKE_CURRENT_SOURCE_DIR}" = "${CMAKE_CURRENT_BINARY_DIR}" || cp ${CMAKE_CURRENT_SOURCE_DIR}/onion.ini  ${CMAKE_CURRENT_BINARY_DIR}/onion.ini
	)

if (${ONION_USE_BINDINGS_CPP})
	add_subdirectory(08-cpp)
endif()

if (${XML2_ENABLED})
add_subdirectory(09-webdav)
else (${XML2_ENABLED})
message(STATUS "No 09-webdav example as no libxml2 available")
endif (${XML2_ENABLED})

