include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_custom_command(
   OUTPUT fileserver_html.c
   COMMAND otemplate ${CMAKE_CURRENT_SOURCE_DIR}/fileserver.html
                 ${CMAKE_CURRENT_BINARY_DIR}/fileserver_html.c
   DEPENDS otemplate ${CMAKE_CURRENT_SOURCE_DIR}/fileserver.html
   )



add_executable(fileserver_otemplate fileserver.c fileserver_html.c)
target_link_libraries(fileserver_otemplate onion_static)

install(TARGETS fileserver_otemplate DESTINATION bin)
