#add_definitions(-D__DEBUG__)

include_directories (${CMAKE_SOURCE_DIR}/src/onion/ ${CMAKE_SOURCE_DIR}/src/handlers/ ${CMAKE_SOURCE_DIR}/src/onionssl/) 

add_custom_command(
   OUTPUT oterm_data.c
   COMMAND opack ${CMAKE_CURRENT_SOURCE_DIR}/index.html ${CMAKE_CURRENT_SOURCE_DIR}/oterm.js 
                 ${CMAKE_CURRENT_SOURCE_DIR}/oterm_input.js ${CMAKE_CURRENT_SOURCE_DIR}/oterm_data.js
                 ${CMAKE_CURRENT_SOURCE_DIR}/oterm_parser.js ${CMAKE_CURRENT_SOURCE_DIR}/jquery-1.4.3.min.js
              -o ${CMAKE_CURRENT_BINARY_DIR}/oterm_data.c
   DEPENDS opack ${CMAKE_CURRENT_SOURCE_DIR}/index.html ${CMAKE_CURRENT_SOURCE_DIR}/oterm.js 
           ${CMAKE_CURRENT_SOURCE_DIR}/oterm_input.js ${CMAKE_CURRENT_SOURCE_DIR}/oterm_data.js
           ${CMAKE_CURRENT_SOURCE_DIR}/oterm_parser.js ${CMAKE_CURRENT_SOURCE_DIR}/jquery-1.4.3.min.js
   )

add_executable(oterm oterm.c oterm_handler.c oterm_data.c)
target_link_libraries(oterm onion_handlers onion_ssl onion util)

