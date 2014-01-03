include_directories (${PROJECT_SOURCE_DIR}/src/) 
include_directories (${PROJECT_SOURCE_DIR}/bindings/cpp/)

add_executable(01-basic 01-basic.cpp)
target_link_libraries(01-basic onion onioncpp)
 

add_executable(02-dict 02-dict.cpp)
target_link_libraries(02-dict onion onioncpp)
 

