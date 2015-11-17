set(kritabrushexport_PART_SRCS
    kis_brush_export.cpp
    )

ki18n_wrap_ui(kritabrushexport_PART_SRCS )

add_library(kritabrushexport MODULE ${kritabrushexport_PART_SRCS})
kcoreaddons_desktop_to_json(kritabrushexport krita_brush_export.desktop)

target_link_libraries(kritabrushexport kritaui)

install(TARGETS kritabrushexport  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})

set(kritabrushimport_PART_SRCS
    kis_brush_import.cpp
    )

ki18n_wrap_ui(kritabrushimport_PART_SRCS )

add_library(kritabrushimport MODULE ${kritabrushimport_PART_SRCS})
kcoreaddons_desktop_to_json(kritabrushimport krita_brush_import.desktop)

target_link_libraries(kritabrushimport kritaui )

install(TARGETS kritabrushimport  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
install( PROGRAMS  krita_brush.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
