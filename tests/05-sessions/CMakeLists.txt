
include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(sessions sessions.c)
target_link_libraries(sessions onion)

