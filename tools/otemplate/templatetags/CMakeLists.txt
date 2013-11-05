add_library(i18n SHARED i18n.c)
target_link_libraries(i18n otemplate_common)
install(TARGETS i18n DESTINATION lib/otemplate/templatetags)
