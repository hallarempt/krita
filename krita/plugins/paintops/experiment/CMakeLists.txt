set(kritaexperimentpaintop_SOURCES
    experiment_paintop_plugin.cpp
    kis_experiment_paintop.cpp
    kis_experimentop_option.cpp
    kis_experiment_paintop_settings.cpp
    kis_experiment_paintop_settings_widget.cpp
    )

ki18n_wrap_ui(kritaexperimentpaintop_SOURCES wdgexperimentoptions.ui )

add_library(kritaexperimentpaintop MODULE ${kritaexperimentpaintop_SOURCES})
kcoreaddons_desktop_to_json(kritaexperimentpaintop kritaexperimentpaintop.desktop)

target_link_libraries(kritaexperimentpaintop kritalibpaintop)

install(TARGETS kritaexperimentpaintop  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
install( FILES
    krita-experiment.png DESTINATION ${DATA_INSTALL_DIR}/krita/images)


