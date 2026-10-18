cmake_minimum_required(VERSION 3.21...4.2)

project(spsc-queue-stages LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    message(FATAL_ERROR "x86-64 only (rdtsc, lfence, prefetchw, clflushopt)")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(SPSC_ARCH "native" CACHE STRING "-march for tuned stages, e.g. znver2")

find_package(Threads REQUIRED)
enable_testing()

# spsc_add_queue(<stage> <plain|tuned>): builds <stage>/ring_buffer.cpp as the
# library <stage>.
function(spsc_add_queue stage build)
    add_library(${stage} STATIC ring_buffer.cpp)
    target_include_directories(${stage} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${stage} PUBLIC spsc_common spsc_${build})
    set(lto OFF)
    if(build STREQUAL "tuned")
        set(lto ON)
    endif()
    set_target_properties(${stage} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ${lto})
endfunction()

set(SPSC_STAGES
    v0_mutex
    v1_atomics
    v2_padding
    v3_cached_cursors
    v4_build_flags
    v5_slot_ticket
    v6_narrow_barriers)

add_subdirectory(common)
foreach(stage IN LISTS SPSC_STAGES)
    add_subdirectory(${stage})
endforeach()
add_subdirectory(bench)
