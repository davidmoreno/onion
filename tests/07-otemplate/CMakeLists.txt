
include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_custom_command(
   OUTPUT test_html.c
   COMMAND otemplate ${CMAKE_CURRENT_SOURCE_DIR}/test.html
                 ${CMAKE_CURRENT_BINARY_DIR}/test_html.c
   DEPENDS otemplate ${CMAKE_CURRENT_SOURCE_DIR}/test.html 
   )


add_executable(01-test 01-test.c test_html.c)
target_link_libraries(01-test onion onion_handlers)
