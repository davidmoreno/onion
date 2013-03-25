include_directories(${CMAKE_SOURCE_DIR}/src/)
add_library(onioncpp SHARED handler.cpp extrahandlers.cpp)
add_library(onioncpp_static STATIC handler.cpp extrahandlers.cpp)

SET(INCLUDES_ONIONCPP onion.hpp dict.hpp request.hpp response.hpp url.hpp handler.hpp extrahandlers.hpp)
MESSAGE("Found include files ${INCLUDES_ONIONCPP}")

install(FILES ${INCLUDES_ONIONCPP} DESTINATION ${INCLUDEDIR})
install(TARGETS onioncpp EXPORT onioncpp DESTINATION ${LIBDIR})
