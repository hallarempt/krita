
set(kritacolorrange_SOURCES colorrange.cc dlg_colorrange.cc )


ki18n_wrap_ui(kritacolorrange_SOURCES wdg_colorrange.ui )

add_library(kritacolorrange MODULE ${kritacolorrange_SOURCES})
kcoreaddons_desktop_to_json(kritacolorrange kritacolorrange.desktop)


target_link_libraries(kritacolorrange kritaui)

install(TARGETS kritacolorrange  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES  colorrange.rc  DESTINATION ${DATA_INSTALL_DIR}/kritaplugins)
