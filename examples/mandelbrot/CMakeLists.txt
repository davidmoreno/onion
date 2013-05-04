include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_custom_command(
   OUTPUT mandel_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/mandel.html
                 ${CMAKE_CURRENT_BINARY_DIR}/mandel_html.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/mandel.html
   )

add_executable(mandelbrot mandelbrot.cpp mandel_html.c)
target_link_libraries(mandelbrot onion_static onion_extras)
