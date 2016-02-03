include_directories(${PROJECT_SOURCE_DIR}/src/)
add_library(onioncpp SHARED dict.cpp handler.cpp extrahandlers.cpp url.cpp shortcuts.cpp exceptions.cpp)
add_library(onioncpp_static STATIC dict.cpp handler.cpp extrahandlers.cpp url.cpp shortcuts.cpp exceptions.cpp)
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x")

SET(INCLUDES_ONIONCPP onion.hpp dict.hpp request.hpp response.hpp url.hpp handler.hpp extrahandlers.hpp shortcuts.hpp exceptions.hpp listen_point.hpp http.hpp https.hpp mime.hpp)

MESSAGE(STATUS "Found include files ${INCLUDES_ONIONCPP}")

SET_TARGET_PROPERTIES(onioncpp onioncpp_static
	PROPERTIES
		SOVERSION 0
		VERSION ${ONION_VERSION}
	)

install(FILES ${INCLUDES_ONIONCPP} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS onioncpp EXPORT onioncpp DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS onioncpp_static EXPORT onioncpp_static DESTINATION ${CMAKE_INSTALL_LIBDIR})
