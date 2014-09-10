#add_definitions(-D__DEBUG__)

include_directories (${PROJECT_SOURCE_DIR}/src/) 
include_directories (${CMAKE_CURRENT_BINARY_DIR}) 

add_custom_command(
   OUTPUT oterm_data.c
   COMMAND ${OPACK} ${CMAKE_CURRENT_SOURCE_DIR}/static
              -o ${CMAKE_CURRENT_BINARY_DIR}/oterm_data.c
   DEPENDS ${OPACK} ${CMAKE_CURRENT_SOURCE_DIR}/static/*
   )

if(EXISTS /usr/share/javascript/jquery/jquery.min.js)
 set(JQUERY_JS /usr/share/javascript/jquery/jquery.min.js)
 message(STATUS "jQuery at ${JQUERY_JS}")
else(EXISTS /usr/share/javascript/jquery/jquery.min.js)
 message(STATUS "jQuery not found. Using onion provided one.")
endif(EXISTS /usr/share/javascript/jquery/jquery.min.js)

if(JQUERY_JS)
  add_definitions(-DJQUERY_JS=\"${JQUERY_JS}\")
  message(STATUS "Using system provided jquery for oterm")
else(JQUERY_JS)
  message(STATUS "Using onion provided jquery for oterm")
  add_custom_command(
    OUTPUT jquery_js.c
    COMMAND ${OPACK} ${CMAKE_CURRENT_SOURCE_DIR}/jquery.js -o ${CMAKE_CURRENT_BINARY_DIR}/jquery_js.c
    DEPENDS ${OPACK} ${CMAKE_CURRENT_SOURCE_DIR}/jquery.js
    )
  set(JQUERY_JS_C jquery_js.c)
endif(JQUERY_JS)


add_executable(oterm oterm.c oterm_handler.c oterm_data.c ${JQUERY_JS_C})
# util is for pty support.. who choose so generic name?
target_link_libraries(oterm onion_static util)

install(TARGETS oterm DESTINATION ${CMAKE_INSTALL_BINDIR})
