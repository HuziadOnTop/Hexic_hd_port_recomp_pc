# hexic_hd - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(hexic_hd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(HEXICHD_SOURCES
    src/main.cpp
    src/dispatch_tolerance.cpp
    src/crash_diag.cpp
    src/fp_fix.cpp
)

if(WIN32)
    add_executable(hexic_hd WIN32 ${HEXICHD_SOURCES})
else()
    add_executable(hexic_hd ${HEXICHD_SOURCES})
endif()

rexglue_setup_target(hexic_hd GPU_PLUGINS xenos)

# imgui include not propagated through OBJECT->SHARED chain
if(TARGET imgui)
    target_link_libraries(hexic_hd PRIVATE imgui)
endif()

if(WIN32)
    target_link_options(hexic_hd PRIVATE "LINKER:/force:multiple" "LINKER:/STACK:16777216")
    target_link_libraries(hexic_hd PRIVATE dbghelp)
endif()
