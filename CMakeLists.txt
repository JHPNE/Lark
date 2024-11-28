# CMakeLists.txt
cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

project(Drosim VERSION 1.0.0 LANGUAGES CXX)

find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)
# Find OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    if(MINGW)
        # MinGW needs special handling for OpenMP
        set(OpenMP_FLAGS "-fopenmp")
        set(OpenMP_LIBS "-lgomp -lpthread")
    endif()
endif()
# Specify C++ standard globally
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use Ninja as the preferred generator
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "")

# Simplified output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform detection
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
endif()

# Build configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_DEBUG_POSTFIX "d")

# Include FetchContent for dependencies
include(FetchContent)

# Include custom CMake modules - compiler options are handled here
include(cmake/Dependencies.cmake)
include(cmake/CompilerOptions.cmake)

# Enable folders for IDE organization
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add subdirectories
add_subdirectory(DroneSim)
add_subdirectory(EngineDLL)
add_subdirectory(NativeEditor)

# Add this after your add_subdirectory calls
set(SCRIPT_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/scripts")
file(MAKE_DIRECTORY ${SCRIPT_OUTPUT_DIR})

# Copy default scripts if you have any
file(GLOB DEFAULT_SCRIPTS "${CMAKE_SOURCE_DIR}/scripts/*.py")
if(DEFAULT_SCRIPTS)
    file(COPY ${DEFAULT_SCRIPTS} DESTINATION ${SCRIPT_OUTPUT_DIR})
endif()

add_dependencies(DroSimEditor EngineDLL)