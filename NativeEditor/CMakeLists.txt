# NativeEditor/CMakeLists.txt
set(PROJECT_NAME DroSimEditor)

# Collect source files
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
        ${glad_SOURCE_DIR}/include
        ${glfw_SOURCE_DIR}/include
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${tinyxml2_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
        imgui
        glad
        glfw
        tinyxml2
        glm
        DroneSim
        EngineDLL
)

# Set properties
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            UNICODE
            _UNICODE
    )
endif()