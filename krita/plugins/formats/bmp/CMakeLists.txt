set(kritabmpexport_SOURCES
    kis_bmp_export.cpp
    )

ki18n_wrap_ui(kritabmpexport_SOURCES )

add_library(kritabmpexport MODULE ${kritabmpexport_SOURCES})
kcoreaddons_desktop_to_json(kritabmpexport krita_bmp_export.desktop)

target_link_libraries(kritabmpexport kritaui)

install(TARGETS kritabmpexport  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})

set(kritabmpimport_SOURCES
    kis_bmp_import.cpp
    )

ki18n_wrap_ui(kritabmpimport_SOURCES )

add_library(kritabmpimport MODULE ${kritabmpimport_SOURCES})
kcoreaddons_desktop_to_json(kritabmpimport krita_bmp_import.desktop)

target_link_libraries(kritabmpimport kritaui)

install(TARGETS kritabmpimport  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})

install( PROGRAMS  krita_bmp.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
