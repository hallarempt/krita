
set(KRITA_PALETTEDOCKER_SOURCES colorsetchooser.cpp palettemodel.cpp palettedocker.cpp palettedocker_dock.cpp )

ki18n_wrap_ui(KRITA_PALETTEDOCKER_SOURCES
    wdgpalettedock.ui
)

add_library(kritapalettedocker MODULE ${KRITA_PALETTEDOCKER_SOURCES})
kcoreaddons_desktop_to_json(kritapalettedocker krita_palettedocker.desktop)

target_link_libraries(kritapalettedocker kritaui)

install(TARGETS kritapalettedocker  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
