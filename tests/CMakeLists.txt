include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_subdirectory(01-internal)

add_subdirectory(02-fullserver)

add_subdirectory(03-methods)

add_subdirectory(04-prerecorded)

add_subdirectory(05-sessions)

#add_subdirectory(06-handlers)

if (OTEMPLATE)
add_subdirectory(07-otemplate)
endif (OTEMPLATE)

ADD_CUSTOM_TARGET(auto.sh ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/auto.sh
	COMMAND [ "${CMAKE_CURRENT_SOURCE_DIR}" = "${CMAKE_CURRENT_BINARY_DIR}" ] || cp ${CMAKE_CURRENT_SOURCE_DIR}/auto.sh  ${CMAKE_CURRENT_BINARY_DIR}/auto.sh
	)

add_subdirectory(08-cpp)