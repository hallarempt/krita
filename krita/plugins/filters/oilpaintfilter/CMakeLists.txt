
set(kritaoilpaintfilter_SOURCES kis_oilpaint_filter_plugin.cpp kis_oilpaint_filter.cpp )


add_library(kritaoilpaintfilter MODULE ${kritaoilpaintfilter_SOURCES})
kcoreaddons_desktop_to_json(kritaoilpaintfilter kritaoilpaintfilter.desktop)



target_link_libraries(kritaoilpaintfilter kritaui)

install(TARGETS kritaoilpaintfilter  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
