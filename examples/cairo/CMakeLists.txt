include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(cairo_example cairo.c)
target_link_libraries(cairo_example onion onion_extras ${CAIRO_LIBRARIES} m)

set_target_properties(cairo_example PROPERTIES OUTPUT_NAME "onion-cairo_example")
install(TARGETS cairo_example DESTINATION ${CMAKE_INSTALL_BINDIR})
