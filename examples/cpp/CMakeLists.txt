SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x")

include_directories (${PROJECT_SOURCE_DIR}/src/) 
include_directories (${PROJECT_SOURCE_DIR}/src/bindings/cpp/)

add_executable(cpp main.cpp)
target_link_libraries(cpp onion onioncpp)
