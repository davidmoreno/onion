
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


add_library(onion_handlers_static STATIC static.c exportlocal.c opack.c path.c internal_status.c
																	${PAM} ${WEBDAV})
target_link_libraries(onion_handlers_static ${PAMLIBS} ${XMLLIBS})

SET(INCLUDES_HANDLERS static.h exportlocal.h auth_pam.h opack.h path.h webdav.h internal_status.h)
MESSAGE(STATUS "Found include files ${INCLUDES_HANDLERS}")

SET_TARGET_PROPERTIES(onion_handlers_static
	PROPERTIES
		SOVERSION 0
		VERSION ${ONION_VERSION}
	)


install(FILES ${INCLUDES_HANDLERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
