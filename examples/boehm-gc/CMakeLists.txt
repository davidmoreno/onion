include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(boehm-gc boehm-gc.c)
target_link_libraries(boehm-gc onion gc)




