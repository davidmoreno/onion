include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_custom_command(
   OUTPUT fileserver_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/fileserver.html
                 ${CMAKE_CURRENT_BINARY_DIR}/fileserver_html.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/fileserver.html
   )


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


add_executable(fileserver_otemplate fileserver.c fileserver_html.c)
target_link_libraries(fileserver_otemplate onion)

install(TARGETS fileserver_otemplate DESTINATION ${BINDIR})

