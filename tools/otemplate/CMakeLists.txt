include_directories (${PROJECT_SOURCE_DIR}/src) 

remove_definitions(-DHAVE_PTHREADS)
remove_definitions(-DHAVE_GNUTLS)

add_executable(otemplate otemplate.c parser.c tags.c variables.c list.c functions.c tag_builtins.c load.c
							../../src/onion/log.c ../../src/onion/block.c ../../src/onion/codecs.c ../../src/onion/dict.c)

if (CMAKE_SYSTEM_NAME  STREQUAL "Linux")
  target_link_libraries(otemplate dl)
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

install(TARGETS otemplate DESTINATION bin)

add_subdirectory(templatetags)
