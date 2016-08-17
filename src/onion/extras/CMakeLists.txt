
include_directories (${PROJECT_SOURCE_DIR}/src/)


if (${PNG_ENABLED})
	set(PNG png.c)
else (${PNG_ENABLED})
	set(PNG "")
endif  (${PNG_ENABLED})

if (${JPEG_ENABLED})
	set(JPEG jpeg.c)
else (${JPEG_ENABLED})
	set(JPEG "")
endif  (${JPEG_ENABLED})

add_library(onion_extras STATIC ${PNG} ${JPEG})

if (${PNG_ENABLED})
target_link_libraries(onion_extras ${PNG_LIBRARIES})
endif (${PNG_ENABLED})

if (${JPEG_ENABLED})
target_link_libraries(onion_extras ${JPEG_LIBRARIES})
endif (${JPEG_ENABLED})

SET(INCLUDES_EXTRAS png.h jpeg.h)
MESSAGE(STATUS "Found include files ${INCLUDES_EXTRAS}")


SET_TARGET_PROPERTIES(onion_extras
	PROPERTIES
		SOVERSION 0
		VERSION ${ONION_VERSION}
	)


install(FILES ${INCLUDES_EXTRAS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/extras/)
install(TARGETS onion_extras EXPORT onion_extras DESTINATION ${CMAKE_INSTALL_LIBDIR})
if (${ARM})
elseif (${ARM})
install(EXPORT onion_extras DESTINATION share/cmake)
endif (${ARM})
