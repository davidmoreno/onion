SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x")


include_directories (${PROJECT_SOURCE_DIR}/src/) 
include_directories (${PROJECT_SOURCE_DIR}/src/bindings/cpp/)

add_executable(01-basic 01-basic.cpp)
target_link_libraries(01-basic onion onioncpp)
 

add_executable(02-dict 02-dict.cpp)
target_link_libraries(02-dict onion onioncpp)
 
add_executable(03-urls 03-urls.cpp)
target_link_libraries(03-urls onion onioncpp)

