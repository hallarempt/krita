set(kritaseparatechannels_SOURCES kis_channel_separator.cc dlg_separate.cc kis_separate_channels_plugin.cc )

ki18n_wrap_ui(kritaseparatechannels_SOURCES wdg_separations.ui )
add_library(kritaseparatechannels MODULE ${kritaseparatechannels_SOURCES})
kcoreaddons_desktop_to_json(kritaseparatechannels kritaseparatechannels.desktop)

target_link_libraries(kritaseparatechannels kritaui)

install(TARGETS kritaseparatechannels  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
install( FILES  imageseparate.rc  DESTINATION ${DATA_INSTALL_DIR}/kritaplugins)
