project(kritapigment)

# we have to repeat platform specifics from top-level
if (WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/winquirks)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
    set(WIN32_PLATFORM_NET_LIBS ws2_32.lib netapi32.lib)
endif ()

include_directories( ${PIGMENT_INCLUDES}  ${Boost_INCLUDE_DIR})

set(FILE_OPENEXR_SOURCES)
set(LINK_OPENEXR_LIB)
if(OPENEXR_FOUND)
    include_directories(${OPENEXR_INCLUDE_DIR})
    set(LINK_OPENEXR_LIB ${OPENEXR_LIBRARIES})
    add_definitions(${OPENEXR_DEFINITIONS})
endif()

set(LINK_VC_LIB)

if(HAVE_VC)
    include_directories(${Vc_INCLUDE_DIR})
    set(LINK_VC_LIB ${Vc_LIBRARIES})
    kde_enable_exceptions()
    ko_compile_for_all_implementations_no_scalar(__per_arch_factory_objs compositeops/KoOptimizedCompositeOpFactoryPerArch.cpp)

    message("Following objects are generated from the per-arch lib")
    message(${__per_arch_factory_objs})
endif()

add_subdirectory(tests)
add_subdirectory(benchmarks)

set(kritapigment_SRCS
    DebugPigment.cpp
    KoBasicHistogramProducers.cpp
    KoColor.cpp
    KoColorDisplayRendererInterface.cpp
    KoColorConversionAlphaTransformation.cpp
    KoColorConversionCache.cpp
    KoColorConversions.cpp
    KoColorConversionSystem.cpp
    KoColorConversionTransformation.cpp
    KoColorConversionTransformationFactory.cpp
    KoColorModelStandardIds.cpp
    KoColorProfile.cpp
    KoColorSpace.cpp
    KoColorSpaceEngine.cpp
    KoColorSpaceFactory.cpp
    KoColorSpaceMaths.cpp
    KoColorSpaceRegistry.cpp
    KoColorTransformation.cpp
    KoColorTransformationFactory.cpp
    KoColorTransformationFactoryRegistry.cpp
    KoCompositeColorTransformation.cpp
    KoCompositeOp.cpp
    KoCompositeOpRegistry.cpp
    KoCopyColorConversionTransformation.cpp
    KoFallBackColorTransformation.cpp
    KoHistogramProducer.cpp
    KoMultipleColorConversionTransformation.cpp
    KoUniqueNumberForIdServer.cpp
    colorspaces/KoAlphaColorSpace.cpp
    colorspaces/KoLabColorSpace.cpp
    colorspaces/KoRgbU16ColorSpace.cpp
    colorspaces/KoRgbU8ColorSpace.cpp
    colorspaces/KoSimpleColorSpaceEngine.cpp
    compositeops/KoOptimizedCompositeOpFactory.cpp
    compositeops/KoOptimizedCompositeOpFactoryPerArch_Scalar.cpp
    ${__per_arch_factory_objs}
    colorprofiles/KoDummyColorProfile.cpp
    resources/KoAbstractGradient.cpp
    resources/KoColorSet.cpp
    resources/KoPattern.cpp
    resources/KoResource.cpp
    resources/KoMD5Generator.cpp
    resources/KoHashGeneratorProvider.cpp
    resources/KoStopGradient.cpp
    resources/KoSegmentGradient.cpp
)

set (EXTRA_LIBRARIES ${LINK_OPENEXR_LIB} ${LINK_VC_LIB})

if(MSVC OR (WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
  # avoid "cannot open file 'LIBC.lib'" error
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:LIBC.LIB")
endif()

add_library(kritapigment SHARED ${kritapigment_SRCS})
generate_export_header(kritapigment)

if (HAVE_VC)
    target_link_libraries(kritapigment KF5::I18n KF5::ConfigCore)
    target_link_libraries(kritapigment LINK_INTERFACE_LIBRARIES KF5::I18n )
endif()

target_link_libraries(
    kritapigment
    kritaplugin
    ${EXTRA_LIBRARIES}
    KF5::I18n 
    Qt5::Gui
    Qt5::Xml
    ${WIN32_PLATFORM_NET_LIBS}
    )

target_link_libraries(
    kritapigment LINK_INTERFACE_LIBRARIES kritaplugin Qt5::Xml Qt5::Gui ${EXTRA_LIBRARIES}
)

set_target_properties(kritapigment PROPERTIES
    VERSION ${GENERIC_CALLIGRA_LIB_VERSION} SOVERSION ${GENERIC_CALLIGRA_LIB_SOVERSION}
)

install(TARGETS kritapigment  ${INSTALL_TARGETS_DEFAULT_ARGS})
