add_library(spsc_common INTERFACE)
target_include_directories(spsc_common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(spsc_common INTERFACE cxx_std_23)
target_compile_options(spsc_common INTERFACE -Wall -Wextra -Wpedantic)

add_library(spsc_plain INTERFACE)
target_compile_options(spsc_plain INTERFACE -O2)

add_library(spsc_tuned INTERFACE)
target_compile_options(spsc_tuned INTERFACE -O3 -march=${SPSC_ARCH})
target_link_options(spsc_tuned INTERFACE -O3 -march=${SPSC_ARCH})
