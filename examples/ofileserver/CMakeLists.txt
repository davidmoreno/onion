include_directories (${PROJECT_SOURCE_DIR}/src/) 
find_program(HAVE_MSGFMT msgfmt)

add_custom_command(
   OUTPUT fileserver_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/fileserver.html
                 ${CMAKE_CURRENT_BINARY_DIR}/fileserver_html.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/fileserver.html
   )

if(HAVE_MSGFMT)
add_custom_target(
   fo_zh ALL
   COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/locale/LC_MESSAGES/
   COMMAND msgfmt  ${CMAKE_CURRENT_SOURCE_DIR}/zh.po 
                 -o ${CMAKE_CURRENT_BINARY_DIR}/locale/LC_MESSAGES/zh.mo
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/zh.po
   )
add_custom_target(
   fo_es ALL
   COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/locale/LC_MESSAGES/
   COMMAND msgfmt  ${CMAKE_CURRENT_SOURCE_DIR}/es.po 
                 -o ${CMAKE_CURRENT_BINARY_DIR}/locale/LC_MESSAGES/es.mo
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/es.po
   )
add_custom_target(
   fo_fr ALL
   COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/locale/LC_MESSAGES/
   COMMAND msgfmt  ${CMAKE_CURRENT_SOURCE_DIR}/fr.po 
                 -o ${CMAKE_CURRENT_BINARY_DIR}/locale/LC_MESSAGES/fr.mo
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fr.po
   )
add_custom_target(
   fo_pl ALL
   COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/locale/LC_MESSAGES/
   COMMAND msgfmt  ${CMAKE_CURRENT_SOURCE_DIR}/pl.po 
                 -o ${CMAKE_CURRENT_BINARY_DIR}/locale/LC_MESSAGES/pl.mo
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pl.po
   )
endif(HAVE_MSGFMT)


add_executable(ofileserver fileserver.c fileserver_html.c)
target_link_libraries(ofileserver onion_static m )

find_package(ZLIB)
if (${ZLIB_FOUND})
   target_link_libraries(ofileserver ${ZLIB_LIBRARIES})
endif(${ZLIB_FOUND})

set_target_properties(ofileserver PROPERTIES OUTPUT_NAME "onion-ofileserver")
install(TARGETS ofileserver	DESTINATION ${CMAKE_INSTALL_BINDIR})

