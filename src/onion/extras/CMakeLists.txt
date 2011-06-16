
include_directories (${CMAKE_SOURCE_DIR}/src/) 


if (${PNG_ENABLED})
	set(PNG png.c)
else (${PNG_ENABLED})
	set(PNG "")
endif  (${PNG_ENABLED})


add_library(onion_extras STATIC ${PNG})

if (${PNG_ENABLED})
target_link_libraries(onion_extras ${PNG_LIB})
endif (${PNG_ENABLED})

SET(INCLUDES_EXTRAS png.h)
MESSAGE("Found include files ${INCLUDES_EXTRAS}")

install(FILES ${INCLUDES_EXTRAS} DESTINATION ${INCLUDEDIR}/extras/)
install(TARGETS onion_extras EXPORT onion_extras DESTINATION ${LIBDIR})
if (${ARM})
elseif (${ARM})
install(EXPORT onion_extras DESTINATION share/cmake)
endif (${ARM})
