#add_definitions(-D__DEBUG__)

include_directories (${PROJECT_SOURCE_DIR}/src/) 
include_directories (${CMAKE_CURRENT_BINARY_DIR}) 

add_custom_command(
   OUTPUT oterm_data.c
   COMMAND ${OPACK} ${CMAKE_CURRENT_SOURCE_DIR}/static
              -o ${CMAKE_CURRENT_BINARY_DIR}/oterm_data.c
   DEPENDS ${OPACK} ${CMAKE_CURRENT_SOURCE_DIR}/static/*
   )

add_executable(oterm oterm.c oterm_handler.c oterm_data.c)
# util is for pty support.. who choose so generic name?
target_link_libraries(oterm onion_handlers_static onion_static util)

install(TARGETS oterm DESTINATION ${CMAKE_INSTALL_BINDIR})
