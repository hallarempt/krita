

set(kritafastcolortransferfilter_SOURCES
    fastcolortransfer.cpp
    kis_wdg_fastcolortransfer.cpp
    )


ki18n_wrap_ui(kritafastcolortransferfilter_SOURCES
    wdgfastcolortransfer.ui
    )

add_library(kritafastcolortransferfilter MODULE ${kritafastcolortransferfilter_SOURCES})
kcoreaddons_desktop_to_json(kritafastcolortransferfilter kritafastcolortransfer.desktop)


target_link_libraries(kritafastcolortransferfilter kritaui)

install(TARGETS kritafastcolortransferfilter  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
