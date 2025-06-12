cmake_minimum_required(VERSION 3.22)

project(gm_plugin
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

add_library(${PROJECT_NAME} SHARED ${PLUGIN_SOURCES})


# Make sure the plugin can see your engine's headers
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

# Make sure the plugin can see the engine's public API
target_include_directories(${PROJECT_NAME} PUBLIC  ${CMAKE_SOURCE_DIR}/src/api)

# Make sure the plugin can see the entt
target_include_directories(${PROJECT_NAME} PUBLIC  ${CMAKE_SOURCE_DIR}/external/entt)

# Set C++ standard
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${PROJECT_ASSETS}plugins/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        "${PROJECT_ASSETS}plugins/"
)
