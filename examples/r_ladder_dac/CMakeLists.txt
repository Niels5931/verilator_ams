cmake_minimum_required(VERSION 3.19)
project(ams_r_ladder_dac_example LANGUAGES CXX)

# Pull in the core AMS library from the repo root.
get_filename_component(AMS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
add_subdirectory(${AMS_ROOT} ${CMAKE_BINARY_DIR}/_ams_root)

# --- Verilator ---
find_package(verilator REQUIRED)

set(VERILOG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/verilog/ams_top.sv
    ${CMAKE_CURRENT_SOURCE_DIR}/verilog/dac_driver.sv
)

add_executable(ams_sim_r_ladder_dac main.cpp)

verilate(ams_sim_r_ladder_dac
    SOURCES ${VERILOG_SOURCES}
    TOP_MODULE ams_top
    VERILATOR_ARGS --trace --assert
)

target_link_libraries(ams_sim_r_ladder_dac PRIVATE ams_lib)

target_include_directories(ams_sim_r_ladder_dac PRIVATE
    ${AMS_ROOT}/include
)
