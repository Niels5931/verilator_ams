cmake_minimum_required(VERSION 3.19)
project(ams_diff_pair_example LANGUAGES CXX)

# Pull in the core AMS library from the repo root.
get_filename_component(AMS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
add_subdirectory(${AMS_ROOT} ${CMAKE_BINARY_DIR}/_ams_root)

# --- Verilator ---
find_package(verilator REQUIRED)

set(VERILOG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/verilog/ams_top.sv
    ${CMAKE_CURRENT_SOURCE_DIR}/verilog/square_wave_driver.sv
)

add_executable(ams_sim_diff_pair main.cpp)

verilate(ams_sim_diff_pair
    SOURCES ${VERILOG_SOURCES}
    TOP_MODULE ams_top
    VERILATOR_ARGS --trace --assert
)

target_link_libraries(ams_sim_diff_pair PRIVATE ams_lib)

target_include_directories(ams_sim_diff_pair PRIVATE
    ${AMS_ROOT}/include
)
