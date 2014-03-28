add_library(i18n SHARED i18n.c)

install(TARGETS i18n DESTINATION ${CMAKE_INSTALL_LIBDIR}/otemplate/templatetags)
