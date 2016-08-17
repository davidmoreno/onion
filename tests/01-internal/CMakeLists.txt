add_executable(01-hash 01-hash.c)
target_link_libraries(01-hash onion)
add_test(internal-hash 01-hash)

add_executable(02-request 02-request.c buffer_listen_point.c)
target_link_libraries(02-request onion)
add_test(internal-request 02-request)

add_executable(03-response 03-response.c buffer_listen_point.c)
target_link_libraries(03-response onion)
add_test(internal-response 03-response)

add_executable(04-handler 04-handler.c buffer_listen_point.c)
target_link_libraries(04-handler onion)
add_test(internal-handler 04-handler)

add_executable(05-server 05-server.c buffer_listen_point.c)
target_link_libraries(05-server onion)
add_test(internal-server 05-server)

if (GNUTLS_ENABLED)
IF (CURL)
message(STATUS "Looking for certool")
FIND_PROGRAM(CERTTOOL certtool)
IF (CERTTOOL)
message(STATUS "Found certool compiling all tests")
add_custom_command(
   OUTPUT mycert.pem
   COMMAND
		${CERTTOOL} --generate-privkey --outfile mycert.key &&
		${CERTTOOL} --generate-self-signed --load-privkey mycert.key --outfile mycert.crt --template ${CMAKE_CURRENT_SOURCE_DIR}/mycert.cfg  &&
		cat mycert.crt mycert.key > mycert.pem
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mycert.cfg
   )


if(PTHREADS)
	add_executable(06-onion 06-onion.c utils.c mycert.pem)
	target_link_libraries(06-onion onion ${CURL_LIBRARIES})
	add_test(internal-onion 06-onion)
endif(PTHREADS)

ELSE(CERTTOOL)
message(STATUS "Not found certool not compiling 06-server")
ENDIF (CERTTOOL)
ENDIF (CURL)
endif(GNUTLS_ENABLED)

add_executable(07-codecs 07-codecs.c)
target_link_libraries(07-codecs onion)
add_test(internal-codecs 07-codecs)

add_executable(08-post 08-post.c buffer_listen_point.c)
target_link_libraries(08-post onion)
add_test(internal-post 08-post)

add_executable(09-block 09-block.c)
target_link_libraries(09-block onion)
add_test(internal-block 09-block)

add_executable(10-mime 10-mime.c)
target_link_libraries(10-mime onion)
add_test(internal-mime 10-mime)

add_executable(11-sessions 11-sessions.c buffer_listen_point.c)
target_link_libraries(11-sessions onion)
add_test(internal-sessions 11-sessions)

IF (CURL)
add_executable(12-cloexec 12-cloexec.c)
target_link_libraries(12-cloexec onion ${CURL_LIBRARIES})
add_test(internal-cloexec 12-cloexec)
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
add_test(internal-otemplates 13-otemplates)
endif (OTEMPLATE)

add_executable(14-websockets 14-websockets.c buffer_listen_point.c)
target_link_libraries(14-websockets onion)
add_test(internal-websockets 14-websockets)

if(PTHREADS)
	add_executable(15-post-no-type 15-post-no-type.c)
	target_link_libraries(15-post-no-type onion)
	add_test(internal-post-no-type 15-post-no-type)
endif(PTHREADS)

add_executable(16-url 16-url.c buffer_listen_point.c)
target_link_libraries(16-url onion)
add_test(internal-url 16-url)

add_executable(17-permissions 17-permissions.c)
target_link_libraries(17-permissions onion)
add_test(internal-permissions 17-permissions)

if(PTHREADS)
	add_executable(18-listen_stop 18-listen_stop.c utils.c)
	target_link_libraries(18-listen_stop onion)
	add_test(listen_stop 18-listen_stop)
endif(PTHREADS)

add_executable(19-random 19-random.c)
target_link_libraries(19-random onion)
add_test(random 19-random)

if (SQLITE3_ENABLED)
 add_executable(20-session_sqlite3 20-session_sqlite3.c)
 target_link_libraries(20-session_sqlite3 onion)
 add_test(session_sqlite3 20-session_sqlite3)
endif(SQLITE3_ENABLED)

add_executable(21-version 21-version.c)
target_link_libraries(21-version onion)
add_test(version 21-version)
