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

# --- yaml-cpp (for config parsing) ---
include(FetchContent)
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.9.0
)
FetchContent_MakeAvailable(yaml-cpp)

# --- Core AMS library ---
add_library(ams_lib STATIC
    src/ngspice_interface.cpp
)
target_include_directories(ams_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NGSPICE_INCLUDE_DIR}
)
target_link_libraries(ams_lib PUBLIC ${NGSPICE_LIBRARY} yaml-cpp::yaml-cpp)

# --- Examples ---
option(AMS_BUILD_EXAMPLES "Build example AMS co-simulation projects" ON)
if(AMS_BUILD_EXAMPLES)
    add_subdirectory(examples/diff_pair)
endif()