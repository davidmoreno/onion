include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_custom_command(
   OUTPUT otop_data.c
   COMMAND ${OPACK} ${CMAKE_CURRENT_SOURCE_DIR}/index.html -o ${CMAKE_CURRENT_BINARY_DIR}/otop_data.c
   DEPENDS ${OPACK} ${CMAKE_CURRENT_SOURCE_DIR}/index.html
   )

add_executable(otop otop.c otop_data.c)
target_link_libraries(otop onion  )

if(${ONION_POLLER} STREQUAL "libev")
	target_link_libraries(otop ${LIBEV_LIBRARIES})
endif()

install(TARGETS otop DESTINATION ${CMAKE_INSTALL_BINDIR})
