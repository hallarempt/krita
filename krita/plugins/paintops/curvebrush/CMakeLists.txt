set(kritacurvepaintop_SOURCES
      curve_brush.cpp
      curve_paintop_plugin.cpp
      kis_curve_paintop.cpp
      kis_curve_paintop_settings.cpp
      kis_curve_line_option.cpp
      kis_linewidth_option.cpp
      kis_curves_opacity_option.cpp
      kis_curve_paintop_settings_widget.cpp
    )

ki18n_wrap_ui(kritacurvepaintop_SOURCES wdgcurveoptions.ui )

add_library(kritacurvepaintop MODULE ${kritacurvepaintop_SOURCES})
kcoreaddons_desktop_to_json(kritacurvepaintop kritacurvepaintop.desktop)

target_link_libraries(kritacurvepaintop kritalibpaintop)

install(TARGETS kritacurvepaintop  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
install( FILES  krita-curve.png DESTINATION ${DATA_INSTALL_DIR}/krita/images)
