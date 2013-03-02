add_executable(01-hash 01-hash.c)
target_link_libraries(01-hash onion)

add_executable(02-request 02-request.c buffer_listen_point.c)
target_link_libraries(02-request onion)

add_executable(03-response 03-response.c buffer_listen_point.c)
target_link_libraries(03-response onion)

add_executable(04-handler 04-handler.c buffer_listen_point.c)
target_link_libraries(04-handler onion_handlers onion)

add_executable(05-server 05-server.c buffer_listen_point.c)
target_link_libraries(05-server onion_handlers onion)

IF (CURL)
add_custom_command(
   OUTPUT mycert.pem
   COMMAND 
		certtool --generate-privkey --outfile mycert.key &&
		certtool --generate-self-signed --load-privkey mycert.key --outfile mycert.crt --template ${CMAKE_CURRENT_SOURCE_DIR}/mycert.cfg  &&
		cat mycert.crt mycert.key > mycert.pem
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mycert.cfg
   )


add_executable(06-onion 06-onion.c mycert.pem)
target_link_libraries(06-onion onion_handlers onion curl)
ENDIF (CURL)

add_executable(07-codecs 07-codecs.c)
target_link_libraries(07-codecs onion_handlers onion)

add_executable(08-post 08-post.c buffer_listen_point.c)
target_link_libraries(08-post onion)

add_executable(09-block 09-block.c)
target_link_libraries(09-block onion)

add_executable(10-mime 10-mime.c)
target_link_libraries(10-mime onion)

add_executable(11-sessions 11-sessions.c buffer_listen_point.c)
target_link_libraries(11-sessions onion)

IF (CURL)
add_executable(12-cloexec 12-cloexec.c)
target_link_libraries(12-cloexec onion curl)
ENDIF (CURL)

if (OTEMPLATE)
add_executable(13-otemplates 13-otemplates.c 13-otemplate_html.c buffer_listen_point.c AGPL_txt.c)
add_custom_command(
   OUTPUT 13-otemplate_html.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/13-otemplate.html
                 ${CMAKE_CURRENT_BINARY_DIR}/13-otemplate_html.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/13-otemplate.html
   )
add_custom_command(
   OUTPUT AGPL_txt.c
   COMMAND ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/../../AGPL.txt
                 ${CMAKE_CURRENT_BINARY_DIR}/AGPL_txt.c
   DEPENDS ${OTEMPLATE} ${CMAKE_CURRENT_SOURCE_DIR}/../../AGPL.txt
   )
target_link_libraries(13-otemplates onion)
endif (OTEMPLATE)

add_executable(14-websockets 14-websockets.c buffer_listen_point.c)
target_link_libraries(14-websockets onion)

add_executable(15-post-no-type 15-post-no-type.c)
target_link_libraries(15-post-no-type onion)

add_executable(16-url 16-url.c buffer_listen_point.c)
target_link_libraries(16-url onion)

