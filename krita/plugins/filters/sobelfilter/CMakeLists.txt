set(kritasobelfilter_SOURCES kis_sobel_filter_plugin.cpp kis_sobel_filter.cpp )
add_library(kritasobelfilter MODULE ${kritasobelfilter_SOURCES})
kcoreaddons_desktop_to_json(kritasobelfilter kritasobelfilter.desktop)
target_link_libraries(kritasobelfilter kritaui)
install(TARGETS kritasobelfilter  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
