cmake_minimum_required(VERSION 3.22)
include(FetchContent)

project(finite
	VERSION 0.1.0
	DESCRIPTION "Raylib Engine using Angle for rendering"
	HOMEPAGE_URL "https://github.com/herodees/finite"
	LANGUAGES CXX
)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Find Python (needed for script)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Required for file globbing and add_custom_command
set(GENERATED_HEADER "${CMAKE_BINARY_DIR}/generated/json_blobs.hpp")

add_custom_command(
    OUTPUT ${GENERATED_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated
    COMMAND python ${CMAKE_SOURCE_DIR}/tools/json_folder_to_combined_header.py
            ${CMAKE_SOURCE_DIR}/prototypes ${GENERATED_HEADER}
    DEPENDS ${CMAKE_SOURCE_DIR}/prototypes/*.json ${CMAKE_SOURCE_DIR}/tools/json_folder_to_combined_header.py
    COMMENT "Generating combined json_blobs.hpp"
)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Define PROJECT_INCLUDE to be the path to the include directory of the project
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src")

if (APPLE)
    set(ANGLE_DIR "${CMAKE_CURRENT_LIST_DIR}/external/angle/macos")
endif()
if (WIN32)
    set(ANGLE_DIR "${CMAKE_CURRENT_LIST_DIR}/external/angle/win64")
    set(ANGLE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/external/angle/include")
    set(ANGLE_BIN_DIR "${ANGLE_DIR}/bin")
    set(ANGLE_LIB_DIR "${ANGLE_DIR}/lib")
endif()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/external/ocornut/imgui" imgui EXCLUDE_FROM_ALL)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/external/raylib-extras/rlImGui" rlImGui EXCLUDE_FROM_ALL)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/external/cdt")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/external/navmesh")

set(BUILD_SHARED_LIBS FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES OFF CACHE BOOL "" FORCE) # don't build the supplied example games
set(PLATFORM "Desktop" CACHE STRING "" FORCE) # build Raylib for Desktop
set(OPENGL_VERSION "ES 3.0" CACHE STRING "" FORCE) # build Raylib for OpenGL ES 2.0
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE) # build Raylib with custom frame control
set(SUPPORT_CUSTOM_FRAME_CONTROL ON CACHE BOOL "" FORCE) # build Raylib with custom frame control
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

if (WIN32)
    target_include_directories(raylib PRIVATE ${ANGLE_INCLUDE_DIR})
endif()

add_executable(${PROJECT_NAME})

if (APPLE)
    # Link required system frameworks for macOS
    find_library(OPENGL_LIBRARY OpenGL)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    find_library(CORE_VIDEO_LIBRARY CoreVideo)

    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})

    find_library(ANGLE_GLESv2_DLL libGLESv2.dylib PATHS ${ANGLE_DIR})
    if(NOT ANGLE_GLESv2_DLL)
        message(FATAL_ERROR "ANGLE_GLESv2_DLL library not found")
    else()
        message(STATUS "Found ANGLE_GLESv2_DLL: ${ANGLE_GLESv2_DLL}")
    endif()

    find_library(ANGLE_EGL_DLL libEGL.dylib PATHS ${ANGLE_DIR})
    if(NOT ANGLE_EGL_DLL)
        message(FATAL_ERROR "ANGLE_EGL_DLL library not found")
    else()
        message(STATUS "Found ANGLE_EGL_DLL: ${ANGLE_EGL_DLL}")
    endif()

    # Define PROJECT_SOURCES as a list of all source files
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib imgui navmesh rlImGui CDT::CDT ${COCOA_LIBRARY} ${OPENGL_LIBRARY} ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY} ${CORE_VIDEO_LIBRARY})

    # Copy the MacOS Angle libraries to the build directory
    file(COPY "${ANGLE_DIR}/" DESTINATION "${CMAKE_BINARY_DIR}")
elseif (WIN32)
    if(MINGW)
        set(MINGW32_LIBRARY mingw32 "-mwindows" CACHE STRING "link flags for MinGW")
    endif()

    # Define PROJECT_SOURCES as a list of all source files

    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE} )

    find_library(ANGLE_GLESv2_DLL libGLESv2.dll.lib PATHS ${ANGLE_LIB_DIR})
    if(NOT ANGLE_GLESv2_DLL)
        message(FATAL_ERROR "ANGLE_GLESv2_DLL library not found")
    else()
        message(STATUS "Found ANGLE_GLESv2_DLL: ${ANGLE_GLESv2_DLL}")
    endif()

    find_library(ANGLE_EGL_DLL libEGL.dll.lib PATHS ${ANGLE_LIB_DIR})
    if(NOT ANGLE_EGL_DLL)
        message(FATAL_ERROR "ANGLE_EGL_DLL library not found")
    else()
        message(STATUS "Found ANGLE_EGL_DLL: ${ANGLE_EGL_DLL}")
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE raylib imgui navmesh rlImGui CDT::CDT ${ANGLE_GLESv2_DLL} ${ANGLE_EGL_DLL})

    # Copy the Win64 Angle libraries to the build directory
    if (MSVC)
        file(COPY "${ANGLE_BIN_DIR}/" DESTINATION "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
    else()
        file(COPY "${ANGLE_BIN_DIR}/" DESTINATION "${CMAKE_BINARY_DIR}")
    endif()
endif()


# Setting ASSETS_PATH
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O0 -DDEBUG")

    # Set the asset path macro to the absolute path on the dev machine
    target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
else()
    # Set the asset path macro in release mode to a relative path that assumes the assets folder is in the same directory as the game executable
    target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="./assets/")
endif()


if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    # Ensure that hot-reload is enabled for VS
    # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
    # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

    # Define static assets to be copied to the build directory
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/assets")
else()
    # Define static assets to be copied to the build directory
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION "${CMAKE_BINARY_DIR}/assets")
endif()

if (${PLATFORM} STREQUAL "Web")
    # Tell Emscripten to build an .html file.
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Os")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -Os -Wall -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 --preload-file assets/ --shell-file ../src/minshell.html")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="/assets/") # Set the asset path macro in release mode to a relative path that assumes the assets folder is in the same directory as the game executable
endif()

target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})

# Add your sources to the target
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.c src/*.cpp src/*.h src/*.hpp )
target_sources(${PROJECT_NAME} PRIVATE ${SRC_FILES})

foreach(FILE ${SRC_FILES})
    file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/src" "${FILE}")
    get_filename_component(GROUP_NAME "${REL_PATH}" DIRECTORY)
    string(REPLACE "/" "\\" GROUP_NAME "${GROUP_NAME}")
    source_group("${GROUP_NAME}" FILES "${FILE}")
endforeach()


add_custom_target(generate_json_blobs ALL DEPENDS ${GENERATED_HEADER})
add_dependencies(${PROJECT_NAME} generate_json_blobs)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/external/portable-file-dialogs")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/external/navmesh")

