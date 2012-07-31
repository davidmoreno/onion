
add_executable(userver userver.c)
target_link_libraries(userver onion_handlers_static onion_static)
