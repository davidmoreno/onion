include_directories (${PROJECT_SOURCE_DIR}/src) 

remove_definitions(-DHAVE_PTHREADS)
remove_definitions(-DHAVE_GNUTLS)

add_executable(otemplate otemplate.c parser.c tags.c variables.c list.c functions.c tag_builtins.c load.c
							../../src/onion/log.c ../../src/onion/block.c ../../src/onion/codecs.c ../../src/onion/dict.c ../../src/onion/low.c 
							../common/updateassets.c)

if (CMAKE_SYSTEM_NAME  STREQUAL "Linux")
  target_link_libraries(otemplate dl)
else (CMAKE_SYSTEM_NAME  STREQUAL "Linux")
  SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--export-dynamic")
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

install(TARGETS otemplate DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(templatetags)
