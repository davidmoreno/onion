
if (PAM_ENABLED)
	if (OPACK)
		add_subdirectory(otop)
		add_subdirectory(oterm)
	endif(OPACK)
	add_subdirectory(basic)
	add_subdirectory(fileserver)
else (PAM_ENABLED)
	if (OPACK)
		add_subdirectory(oterm)
		message(WARNING "Oterm without authentication support! May be very unsecure.")
	else(OPACK)
		message(WARNING "Oterm not compiled as there is no opack.")
	endif (OPACK)

	message(WARNING "Some examples are disabled as you dont have PAM: otop, basic, fileserver.")
endif (PAM_ENABLED)

add_subdirectory(userver)
add_subdirectory(interactive)
add_subdirectory(hello)


if (OTEMPLATE)
	add_subdirectory(ofileserver)
	if (${PNG_ENABLED})
		add_subdirectory(mandelbrot)
	else (${PNG_ENABLED})
		message(WARNING "Some examples are disabled as you dont have libpng: mandelbrot")
	endif (${PNG_ENABLED})
else(OTEMPLATE)
	message(WARNING "Some examples are disabled as you dont have otemplate installed: ofileserver mandelbrot")
endif(OTEMPLATE)

if (${CAIRO_ENABLED})
	add_subdirectory(cairo)
else (${CAIRO_ENABLED})
	message(WARNING "Some examples are disabled as you dont have cairo: cairo")
endif (${CAIRO_ENABLED})



