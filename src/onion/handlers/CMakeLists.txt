
include_directories (${CMAKE_SOURCE_DIR}/src/) 


if (PAM_ENABLED)
	SET(PAM auth_pam.c)
	SET(PAMLIBS pam pam_misc)
else (PAM_ENABLED)
	SET(PAM)
	SET(PAMLIBS)
endif (PAM_ENABLED)

add_library(onion_handlers STATIC static.c exportlocal.c opack.c path.c
																	${PAM} )

target_link_libraries(onion_handlers ${PAMLIBS})

SET(INCLUDES_HANDLERS static.h exportlocal.h auth_pam.h opack.h path.h)
MESSAGE("Found include files ${INCLUDES_HANDLERS}")

install(FILES ${INCLUDES_HANDLERS} DESTINATION ${INCLUDEDIR}/handlers/)
install(TARGETS onion_handlers EXPORT onion_handlers DESTINATION ${LIBDIR})
if (${ARM})
elseif (${ARM})
install(EXPORT onion_handlers DESTINATION share/cmake)
endif (${ARM})

