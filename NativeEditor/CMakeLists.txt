set(PROJECT_NAME DroSimEditor)

# Collect source files
file(GLOB_RECURSE EDITOR_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${EDITOR_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/EngineDLL      # Add direct path to EngineDLL
        ${CMAKE_SOURCE_DIR}/DroneSim       # Add DroneSim root
        ${CMAKE_SOURCE_DIR}/DroneSim/Common
        ${CMAKE_SOURCE_DIR}/DroneSim/Components
        ${CMAKE_SOURCE_DIR}/DroneSim/DroneSimAPI
        ${glad_SOURCE_DIR}/include
        ${glfw_SOURCE_DIR}/include
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${tinyxml2_SOURCE_DIR}
        ${Python_INCLUDE_DIRS}
        ${pybind11_INCLUDE_DIRS}
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
        Python::Python
        pybind11::embed
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

# Copy DLL after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:EngineDLL>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)