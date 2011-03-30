include_directories (${CMAKE_SOURCE_DIR}/src) 

add_executable(opack opack.c)

if ( ${ARM} MATCHES "yes" )
install(TARGETS opack DESTINATION bin)
endif ( ${ARM} MATCHES "yes" )





