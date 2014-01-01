if (GETTEXT_ENABLED)
	add_library(i18n SHARED i18n.c)
else(GETTEXT_ENABLED)
	message (STATUS "Disabling use of gettext")
	add_library(i18n SHARED i18n_fake.c)
endif (GETTEXT_ENABLED)


target_link_libraries(i18n otemplate_common)
install(TARGETS i18n DESTINATION lib/otemplate/templatetags)
