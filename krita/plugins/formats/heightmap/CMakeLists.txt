include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

set(kritaheightmapimport_SOURCES
    kis_heightmap_import.cpp
    )

ki18n_wrap_ui(kritaheightmapimport_SOURCES kis_wdg_options_heightmap.ui )

add_library(kritaheightmapimport MODULE ${kritaheightmapimport_SOURCES})
kcoreaddons_desktop_to_json(kritaheightmapimport krita_heightmap_import.desktop)

target_link_libraries(kritaheightmapimport kritaui )

install(TARGETS kritaheightmapimport  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})

########### next target ###############

set(kritaheightmapexport_SOURCES
    kis_heightmap_export.cpp
    )

ki18n_wrap_ui(kritaheightmapexport_SOURCES kis_wdg_options_heightmap.ui )

add_library(kritaheightmapexport MODULE ${kritaheightmapexport_SOURCES})
kcoreaddons_desktop_to_json(kritaheightmapexport krita_heightmap_export.desktop)

target_link_libraries(kritaheightmapexport kritaui  )

install(TARGETS kritaheightmapexport  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})

########### install files ###############

install( PROGRAMS  krita_heightmap.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
