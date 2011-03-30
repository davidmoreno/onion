include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_executable(interactive interactive.c)

target_link_libraries(interactive onion_static)

if ( ${ARM} MATCHES "yes" )
install(TARGETS interactive DESTINATION bin)
endif ( ${ARM} MATCHES "yes" )

