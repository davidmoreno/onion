include_directories (${CMAKE_SOURCE_DIR}/src) 

add_executable(otemplate otemplate.c parser.c tags.c variables.c list.c functions.c 
							../../src/onion/log.c ../../src/onion/block.c ../../src/onion/codecs.c)

if ( ${ARM} MATCHES "yes" )
install(TARGETS otemplate DESTINATION bin)
endif ( ${ARM} MATCHES "yes" )





