
kde_enable_exceptions()

set(kritaimageenhancement_SOURCES
    imageenhancement.cpp
    kis_simple_noise_reducer.cpp
    kis_wavelet_noise_reduction.cpp
    )

add_library(kritaimageenhancement MODULE ${kritaimageenhancement_SOURCES})
kcoreaddons_desktop_to_json(kritaimageenhancement kritaimageenhancement.desktop)


target_link_libraries(kritaimageenhancement kritaui)

install(TARGETS kritaimageenhancement  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
