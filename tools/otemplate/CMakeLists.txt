include_directories (${CMAKE_SOURCE_DIR}/src) 

add_executable(otemplate otemplate.c block.c list.c code.c functions.c ../../src/onion/log.c)

if ( ${ARM} MATCHES "yes" )
install(TARGETS otemplate DESTINATION bin)
endif ( ${ARM} MATCHES "yes" )





