
SET(INCLUDES_HANDLERS static.h exportlocal.h auth_pam.h opack.h path.h webdav.h internal_status.h)

install(FILES ${INCLUDES_HANDLERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
