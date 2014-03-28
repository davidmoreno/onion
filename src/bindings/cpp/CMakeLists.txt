include_directories(${PROJECT_SOURCE_DIR}/src/)
add_library(onioncpp SHARED dict.cpp handler.cpp extrahandlers.cpp url.cpp)
add_library(onioncpp_static STATIC dict.cpp handler.cpp extrahandlers.cpp url.cpp)
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x")

SET(INCLUDES_ONIONCPP onion.hpp dict.hpp request.hpp response.hpp url.hpp handler.hpp extrahandlers.hpp)
MESSAGE(STATUS "Found include files ${INCLUDES_ONIONCPP}")

install(FILES ${INCLUDES_ONIONCPP} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS onioncpp EXPORT onioncpp DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS onioncpp_static EXPORT onioncpp_static DESTINATION ${CMAKE_INSTALL_LIBDIR})
