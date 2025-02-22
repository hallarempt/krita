add_definitions(-DTRANSLATION_DOMAIN=\"krita_shape_paths\")

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/widgets
    ${FLAKE_INCLUDES}
)

set(pathshapes_SOURCES
    ellipse/EllipseShape.cpp
    ellipse/EllipseShapeFactory.cpp
    ellipse/EllipseShapeConfigWidget.cpp
    ellipse/EllipseShapeConfigCommand.cpp
    spiral/SpiralShape.cpp
    spiral/SpiralShapeFactory.cpp
    spiral/SpiralShapeConfigWidget.cpp
    spiral/SpiralShapeConfigCommand.cpp
    star/StarShape.cpp
    star/StarShapeFactory.cpp
    star/StarShapeConfigWidget.cpp
    star/StarShapeConfigCommand.cpp
    rectangle/RectangleShape.cpp
    rectangle/RectangleShapeFactory.cpp
    rectangle/RectangleShapeConfigWidget.cpp
    rectangle/RectangleShapeConfigCommand.cpp
    enhancedpath/EnhancedPathShape.cpp
    enhancedpath/EnhancedPathShapeFactory.cpp
    enhancedpath/EnhancedPathCommand.cpp
    enhancedpath/EnhancedPathParameter.cpp
    enhancedpath/EnhancedPathFormula.cpp
    enhancedpath/EnhancedPathHandle.cpp
    PathShapesPlugin.cpp
)

ki18n_wrap_ui(pathshapes_SOURCES
    star/StarShapeConfigWidget.ui
    rectangle/RectangleShapeConfigWidget.ui
    ellipse/EllipseShapeConfigWidget.ui
    spiral/SpiralShapeConfigWidget.ui
)

qt5_add_resources(pathshapes_SOURCES pathshapes.qrc)

add_library(krita_shape_paths MODULE ${pathshapes_SOURCES})
kcoreaddons_desktop_to_json(krita_shape_paths calligra_shape_paths.desktop)

target_link_libraries(krita_shape_paths kritaflake kritawidgets)

install(TARGETS krita_shape_paths  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})
