
include_directories (${PROJECT_SOURCE_DIR}/src/) 


if (PAM_ENABLED)
	SET(PAM auth_pam.c)
	SET(PAMLIBS pam pam_misc)
else (PAM_ENABLED)
	SET(PAM)
	SET(PAMLIBS)
endif (PAM_ENABLED)

if (${XML2_ENABLED})
	MESSAGE(STATUS "Compiling WebDAV support") 
	SET(WEBDAV webdav.c)
	SET(XMLLIBS ${XML2_LIB})
	include_directories(${XML2_HEADERS})
endif (${XML2_ENABLED})


add_library(onion_handlers SHARED static.c exportlocal.c opack.c path.c internal_status.c
																	${PAM} ${WEBDAV})
target_link_libraries(onion_handlers ${PAMLIBS} ${XMLLIBS} onion)

add_library(onion_handlers_static STATIC static.c exportlocal.c opack.c path.c internal_status.c
																	${PAM} ${WEBDAV})
target_link_libraries(onion_handlers_static ${PAMLIBS} ${XMLLIBS})



SET(INCLUDES_HANDLERS static.h exportlocal.h auth_pam.h opack.h path.h webdav.h internal_status.h)
MESSAGE(STATUS "Found include files ${INCLUDES_HANDLERS}")

install(FILES ${INCLUDES_HANDLERS} DESTINATION ${INCLUDEDIR}/handlers/)
install(TARGETS onion_handlers EXPORT onion_handlers DESTINATION ${LIBDIR})
install(TARGETS onion_handlers_static EXPORT onion_handlers_static DESTINATION ${LIBDIR})
if (${ARM})
elseif (${ARM})
install(EXPORT onion_handlers_static DESTINATION share/cmake)
endif (${ARM})

