include_directories(${PROJECT_SOURCE_DIR}/src/)
add_library(onioncpp SHARED handler.cpp extrahandlers.cpp)
add_library(onioncpp_static STATIC handler.cpp extrahandlers.cpp)

target_link_libraries(onioncpp onion)
target_link_libraries(onioncpp_static onioncpp_static)

SET(INCLUDES_ONIONCPP onion.hpp dict.hpp request.hpp response.hpp url.hpp handler.hpp extrahandlers.hpp)
MESSAGE(STATUS "Found include files ${INCLUDES_ONIONCPP}")

install(FILES ${INCLUDES_ONIONCPP} DESTINATION ${INCLUDEDIR})
install(TARGETS onioncpp EXPORT onioncpp DESTINATION ${LIBDIR})
install(TARGETS onioncpp_static EXPORT onioncpp_static DESTINATION ${LIBDIR})
