cmake_minimum_required(VERSION 3.22)

project(example_plugin
    VERSION 0.1.0
    LANGUAGES CXX
)

# Collect all source files recursively inside the plugin directory

file(GLOB_RECURSE PLUGIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
    "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
)

foreach(FILE ${PLUGIN_SOURCES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_LIST_DIR}" "${FILE}")
    get_filename_component(GROUP_NAME "${REL_PATH}" DIRECTORY)
    string(REPLACE "/" "\\" GROUP_NAME "${GROUP_NAME}")
    source_group("${GROUP_NAME}" FILES "${FILE}")
endforeach()

# Add external sources (e.g., shared plugin helpers or interface stubs)
list(APPEND PLUGIN_SOURCES "${CMAKE_SOURCE_DIR}/src/api/plugin_main.cpp")

add_library(example_plugin SHARED ${PLUGIN_SOURCES})


# Make sure the plugin can see your engine's headers
target_include_directories(example_plugin PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

# Make sure the plugin can see the engine's public API
target_include_directories(example_plugin PUBLIC  ${CMAKE_SOURCE_DIR}/src/api)

# Set C++ standard
target_compile_features(example_plugin PRIVATE cxx_std_20)

# Optional: Define BUILDING_DLL so you can guard exports if needed
target_compile_definitions(example_plugin PRIVATE BUILDING_DLL=1)

# Remove 'lib' prefix on Linux/macOS for cleaner plugin name (optional)
set_target_properties(example_plugin PROPERTIES
    PREFIX ""
    # Set suffix if you want to force a particular extension, but normally leave it default
    # SUFFIX ".dll" # for Windows, or ".dylib" for macOS, ".so" for Linux
)
target_compile_definitions(example_plugin PRIVATE BUILDING_DLL)
