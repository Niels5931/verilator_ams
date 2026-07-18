cmake_minimum_required(VERSION 3.19)
project(ams LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- ngspice ---
find_library(NGSPICE_LIBRARY
    NAMES ngspice ngspice-0
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
          "$ENV{NGSPICE_HOME}/lib"
)
find_path(NGSPICE_INCLUDE_DIR
    NAMES ngspice/sharedspice.h
    PATHS /usr/include /usr/local/include
          "$ENV{NGSPICE_HOME}/include"
)
if(NOT NGSPICE_LIBRARY)
    message(FATAL_ERROR "ngspice library not found. Set NGSPICE_LIBRARY manually.")
endif()
if(NOT NGSPICE_INCLUDE_DIR)
    message(WARNING "ngspice include dir not found; set NGSPICE_INCLUDE_DIR if needed.")
endif()

# --- yaml-cpp (system-installed; for config parsing) ---
# The Python build driver (tools/ams_sim.py build-core) requires yaml-cpp as a
# system library (or via YAML_CPP_HOME). CMake uses find_package to match that
# expectation rather than vendoring yaml-cpp via FetchContent.
find_package(yaml-cpp REQUIRED)

# --- Core AMS shared library ---
# libams_core.so is the single artifact consumed by the SV UVM / pyuvm flows.
# It mirrors what tools/ams_sim.py build-core produces. There are no per-example
# CMake targets; the supported example (examples/uvm_r_ladder_dac) is built by
# tools/ams_sim.py via Verilator --binary (SV UVM) or the cocotb Makefile (pyuvm).
add_library(ams_core SHARED
    src/ngspice_interface.cpp
    src/ams_dpi.cpp
)
target_include_directories(ams_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NGSPICE_INCLUDE_DIR}
)
target_link_libraries(ams_core PRIVATE ${NGSPICE_LIBRARY} yaml-cpp)