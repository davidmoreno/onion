
include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_custom_command(
   OUTPUT test_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/test.html
                 ${CMAKE_CURRENT_BINARY_DIR}/test_html.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/test.html
   )

add_custom_command(
   OUTPUT external_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/external.html
                 ${CMAKE_CURRENT_BINARY_DIR}/external_html.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/external.html
   )

add_custom_command(
   OUTPUT extended_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/extended.html
                 ${CMAKE_CURRENT_BINARY_DIR}/extended_html.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/extended.html
   )

add_custom_command(
   OUTPUT base_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/base.html
                 ${CMAKE_CURRENT_BINARY_DIR}/base_html.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/base.html
   )


add_executable(01-test 01-test.c test_html.c external_html.c extended_html.c base_html.c)
target_link_libraries(01-test onion_static onion_handlers)
