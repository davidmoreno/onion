
include_directories (${CMAKE_SOURCE_DIR}/src/) 


add_library(onion_extras STATIC png.c)

target_link_libraries(onion_extras png)

SET(INCLUDES_EXTRAS png.h)
MESSAGE("Found include files ${INCLUDES_EXTRAS}")

install(FILES ${INCLUDES_EXTRAS} DESTINATION ${INCLUDEDIR}/extras/)
install(TARGETS onion_extras EXPORT onion_extras DESTINATION ${LIBDIR})
install(EXPORT onion_extras DESTINATION share/cmake)
