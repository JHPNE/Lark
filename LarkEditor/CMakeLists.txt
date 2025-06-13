set(PROJECT_NAME LarkEditor)

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
        ${CMAKE_SOURCE_DIR}/EngineDLL
        ${CMAKE_SOURCE_DIR}/Lark
        ${CMAKE_SOURCE_DIR}/Lark/Common
        ${CMAKE_SOURCE_DIR}/Lark/Components
        ${CMAKE_SOURCE_DIR}/Lark/LarkAPI
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
        Lark
        EngineDLL
        Python::Python
        pybind11::embed
)

# Set common properties
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        BUILD_WITH_INSTALL_RPATH TRUE
)

# Create scripts directory in binary directory (common for all platforms)
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_BINARY_DIR}/bin/scripts"
)

# Platform-specific settings
if(APPLE)
    # macOS specific settings
    set_target_properties(${PROJECT_NAME} PROPERTIES
            INSTALL_RPATH "@loader_path;@executable_path;${Python_RUNTIME_LIBRARY_DIRS}"
    )

    # Copy EngineDLL
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:EngineDLL>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )

elseif(UNIX AND NOT APPLE)
    # Linux specific settings
    set_target_properties(${PROJECT_NAME} PROPERTIES
            INSTALL_RPATH "$ORIGIN:$ORIGIN/lib:${Python_RUNTIME_LIBRARY_DIRS}"
    )

    # Copy EngineDLL
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:EngineDLL>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"

            # For Linux, we might need to copy additional dependencies
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${Python_RUNTIME_LIBRARY_RELEASE}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$(basename ${Python_RUNTIME_LIBRARY_RELEASE})"
    )

    # Add X11 dependency for Linux
    target_link_libraries(${PROJECT_NAME} PRIVATE X11)

elseif(MSVC)
    # Windows specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            UNICODE
            _UNICODE
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:EngineDLL>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${Python_RUNTIME_LIBRARY_RELEASE}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )

    # Set PATH for Windows to find Python DLL
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_ENVIRONMENT
            "PATH=${Python_RUNTIME_LIBRARY_DIRS};$ENV{PATH}")
endif()

# Copy any default scripts if they exist (common for all platforms)
if(EXISTS "${CMAKE_SOURCE_DIR}/scripts")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/scripts"
            "${CMAKE_BINARY_DIR}/bin/scripts"
    )
endif()

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Python_RUNTIME_LIBRARY_RELEASE}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endif()