include_directories (${PROJECT_SOURCE_DIR}/src) 

remove_definitions(-DHAVE_PTHREADS)
remove_definitions(-DHAVE_GNUTLS)

add_library(otemplate_common parser.c tags.c variables.c list.c functions.c tag_builtins.c load.c
							../../src/onion/log.c ../../src/onion/block.c ../../src/onion/codecs.c ../../src/onion/dict.c ../../src/onion/low.c 
							../common/updateassets.c)

add_executable(otemplate otemplate.c)
target_link_libraries(otemplate otemplate_common)

if (CMAKE_SYSTEM_NAME  STREQUAL "Linux")
  target_link_libraries(otemplate dl)
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--export-dynamic")
endif (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")

install(TARGETS otemplate DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(templatetags)
