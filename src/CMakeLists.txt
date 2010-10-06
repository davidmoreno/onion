add_library(onion onion_dict.c onion.c onion_request.c onion_response.c)

target_link_libraries(onion onion_handlers)

add_subdirectory(handlers)
