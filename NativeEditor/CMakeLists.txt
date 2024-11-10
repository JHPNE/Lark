cmake_minimum_required(VERSION 3.15)
project(DroSimEditor VERSION 1.0.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)

# Add GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch Dear ImGui (docking branch)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking    # This specifies the docking branch
)
FetchContent_MakeAvailable(imgui)

# Find packages
find_package(OpenGL REQUIRED)

# Add GLAD
add_library(glad STATIC
    ThirdParty/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    ThirdParty/glad/include
)
target_link_libraries(glad PRIVATE
    ${OPENGL_LIBRARIES}
)

# ImGui source files
add_library(imgui STATIC
    ThirdParty/imgui/imgui.cpp
    ThirdParty/imgui/imgui_demo.cpp
    ThirdParty/imgui/imgui_draw.cpp
    ThirdParty/imgui/imgui_tables.cpp
    ThirdParty/imgui/imgui_widgets.cpp
    ThirdParty/imgui/backends/imgui_impl_glfw.cpp
    ThirdParty/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/backends
)

target_include_directories(imgui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw/include
)

target_link_libraries(imgui PUBLIC
    glad
    glfw
)

# Editor files
file(GLOB_RECURSE EDITOR_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${EDITOR_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui/backends
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    glad
    glfw
)

# Print some debug info
message(STATUS "Project source directory: ${CMAKE_CURRENT_SOURCE_DIR}")