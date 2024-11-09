cmake_minimum_required(VERSION 3.15)
project(DroSimEditor VERSION 1.0.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find packages
find_package(OpenGL REQUIRED)

# Add GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/glfw)

# Add GLAD
add_library(glad STATIC
    ThirdParty/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    ThirdParty/glad/include
)

# ImGui source files
set(IMGUI_SOURCES
    ThirdParty/imgui/imgui.cpp
    ThirdParty/imgui/imgui_demo.cpp
    ThirdParty/imgui/imgui_draw.cpp
    ThirdParty/imgui/imgui_tables.cpp
    ThirdParty/imgui/imgui_widgets.cpp
    ThirdParty/imgui/backends/imgui_impl_glfw.cpp
    ThirdParty/imgui/backends/imgui_impl_opengl3.cpp
)

# Editor source files
set(EDITOR_SOURCES
    Source/Main.cpp
)

# Create executable
add_executable(${PROJECT_NAME} 
    ${EDITOR_SOURCES}
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw/include
    ${OPENGL_INCLUDE_DIR}
)

# Debug print
get_target_property(includeDirs ${PROJECT_NAME} INCLUDE_DIRECTORIES)
message(STATUS "Include directories for ${PROJECT_NAME}:")
foreach(dir ${includeDirs})
    message(STATUS " - ${dir}")
    if(EXISTS "${dir}/imgui/imgui.h")
        message(STATUS "   Found imgui.h")
    endif()
endforeach()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    ${OPENGL_LIBRARIES}
)

# Message to verify paths
message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Looking for main.cpp at: ${CMAKE_CURRENT_SOURCE_DIR}/${EDITOR_SOURCES}")