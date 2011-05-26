
if (PAM_ENABLED)
if (OPACK)
	add_subdirectory(otop)
	add_subdirectory(oterm)
endif(OPACK)
	add_subdirectory(basic)
	add_subdirectory(fileserver)
else (PAM_ENABLED)
	message(WARNING "Some examples are disabled as you dont have PAM: otop, oterm, basic, fileserver")
endif (PAM_ENABLED)

add_subdirectory(userver)
add_subdirectory(interactive)
add_subdirectory(hello)


if (OTEMPLATE)
	add_subdirectory(fileserver_otemplate)
if (${PNG_LIB})
	add_subdirectory(mandelbrot)
endif (${PNG_LIB})
endif(OTEMPLATE)
