# Tests/CMakeLists.txt

# Set a unique project name for the Tests subdirectory (optional but recommended)
# Not strictly necessary if the top-level CMakeLists.txt already defines the project.

# Collect all .cpp and .h files in the src directory
file(GLOB_RECURSE TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Check if any sources were found
if(NOT TEST_SOURCES)
    message(FATAL_ERROR "No source files found for Tests target.")
endif()

# Create the executable target named Tests
add_executable(Tests ${TEST_SOURCES})

# Enable testing support
enable_testing()

# Include directories for Tests
target_include_directories(Tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/EngineDLL/include
        ${CMAKE_SOURCE_DIR}/Lark/include
        # Add other include paths as needed
        ${glad_SOURCE_DIR}/include
        ${glfw_SOURCE_DIR}/include
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${tinyxml2_SOURCE_DIR}
        ${Python_INCLUDE_DIRS}
        ${pybind11_INCLUDE_DIRS}
)

# Link dependencies for Tests
target_link_libraries(Tests PRIVATE
        imgui
        glad
        glfw
        tinyxml2
        glm
        Lark
        EngineDLL
        Python::Python
        pybind11::embed
        gtest
)

# OpenMP configuration (if required)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(Tests PRIVATE OpenMP::OpenMP_CXX)
endif()

# Set target properties
set_target_properties(Tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        BUILD_WITH_INSTALL_RPATH TRUE
)

# Platform-specific settings
if(APPLE)
    set_target_properties(Tests PROPERTIES
            INSTALL_RPATH "@loader_path;@executable_path;${Python_RUNTIME_LIBRARY_DIRS}"
    )

    # Copy EngineDLL after build
    add_custom_command(TARGET Tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:EngineDLL>"
            "$<TARGET_FILE_DIR:Tests>"
    )

elseif(UNIX AND NOT APPLE)
    set_target_properties(Tests PROPERTIES
            INSTALL_RPATH "$ORIGIN:$ORIGIN/lib:${Python_RUNTIME_LIBRARY_DIRS}"
    )

    # Copy EngineDLL and create symlink for Python runtime
    add_custom_command(TARGET Tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:EngineDLL>"
            "$<TARGET_FILE_DIR:Tests>"

            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${Python_RUNTIME_LIBRARY_RELEASE}"
            "$<TARGET_FILE_DIR:Tests>/$(basename ${Python_RUNTIME_LIBRARY_RELEASE})"
    )

    # Link X11 on Linux
    target_link_libraries(Tests PRIVATE X11)

elseif(MSVC)
    target_compile_definitions(Tests PRIVATE
            UNICODE
            _UNICODE
    )

    add_custom_command(TARGET Tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:EngineDLL>"
            "$<TARGET_FILE_DIR:Tests>"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Python_RUNTIME_LIBRARY_RELEASE}"
            "$<TARGET_FILE_DIR:Tests>"
    )

    # Set PATH for Windows to find Python DLL
    set_property(TARGET Tests PROPERTY VS_DEBUGGER_ENVIRONMENT
            "PATH=${Python_RUNTIME_LIBRARY_DIRS};$ENV{PATH}"
    )
endif()

# Copy scripts after build
if(EXISTS "${CMAKE_SOURCE_DIR}/scripts")
    add_custom_command(TARGET Tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/scripts"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scripts"
    )
endif()

# Ensure dependencies are built before Tests
add_dependencies(Tests EngineDLL Lark)

# Include GoogleTest
include(GoogleTest)

# Discover and register tests
gtest_discover_tests(Tests)
