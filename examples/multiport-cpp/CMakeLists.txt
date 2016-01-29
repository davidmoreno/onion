set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x")

include_directories (${PROJECT_SOURCE_DIR}/src/)
include_directories (${PROJECT_SOURCE_DIR}/src/bindings/cpp/)

add_executable(multiport-cpp multiport.cpp)

target_link_libraries(multiport-cpp onioncpp_static onion_static)

