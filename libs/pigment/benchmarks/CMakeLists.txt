
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

########### next target ###############

set(ko_colorspaces_benchmark_SRCS KoColorSpacesBenchmark.cpp)
calligra_add_benchmark(KoColorSpacesBenchmark TESTNAME pigment-benchmarks-KoColorSpacesBenchmark ${ko_colorspaces_benchmark_SRCS})
target_link_libraries(KoColorSpacesBenchmark kritapigment KF5::I18n  Qt5::Test)

set(ko_compositeops_benchmark_SRCS KoCompositeOpsBenchmark.cpp)
calligra_add_benchmark(KoCompositeOpsBenchmark TESTNAME pigment-benchmarks-KoCompositeOpsBenchmark ${ko_compositeops_benchmark_SRCS})
target_link_libraries(KoCompositeOpsBenchmark  kritapigment KF5::I18n  Qt5::Test)

