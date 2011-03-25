
include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_library(onion_handlers STATIC static.c path.c directory.c
																	regexp.c auth_pam.c opack.c
																	url.c)

target_link_libraries(onion_handlers pam pam_misc)

SET(INCLUDES_HANDLERS static.h path.h directory.h regexp.h auth_pam.h opack.h url.h)
MESSAGE("Found include files ${INCLUDES_HANDLERS}")

install(FILES ${INCLUDES_HANDLERS} DESTINATION include/onion/handlers/)
install(TARGETS onion_handlers EXPORT onion_handlers DESTINATION lib)
install(EXPORT onion_handlers DESTINATION share/cmake)
