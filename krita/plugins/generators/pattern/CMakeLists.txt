set(kritapatterngenerator_SOURCES
    patterngenerator.cpp
    kis_wdg_pattern.cpp
    )


ki18n_wrap_ui(kritapatterngenerator_SOURCES
    wdgpatternoptions.ui
    )

add_library(kritapatterngenerator MODULE ${kritapatterngenerator_SOURCES})
kcoreaddons_desktop_to_json(kritapatterngenerator kritapatterngenerator.desktop)

target_link_libraries(kritapatterngenerator kritaui)

install(TARGETS kritapatterngenerator  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
