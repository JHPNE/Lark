# Tests/CMakeLists.txt

# Include GoogleTest module
include(GoogleTest)
include(CTest)

# Collect all .cpp and .h files in the src directory
file(GLOB_RECURSE TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

if(NOT TEST_SOURCES)
    message(FATAL_ERROR "No source files found for Tests target.")
endif()

# Create the main test executable
add_executable(Tests ${TEST_SOURCES})

# Create a separate executable target for visualization
add_executable(VisualizationTest ${TEST_SOURCES})

# Common function to set up target properties and links
function(setup_test_target target_name)
    target_include_directories(${target_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/EngineDLL/include
            ${CMAKE_SOURCE_DIR}/DroneSim/include
            ${glad_SOURCE_DIR}/include
            ${glfw_SOURCE_DIR}/include
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
            ${tinyxml2_SOURCE_DIR}
            ${Python_INCLUDE_DIRS}
            ${pybind11_INCLUDE_DIRS}
    )

    target_link_libraries(${target_name} PRIVATE
            imgui
            glad
            glfw
            tinyxml2
            glm
            DroneSim
            EngineDLL
            Python::Python
            pybind11::embed
            gtest
            gtest_main
    )

    if(OpenMP_CXX_FOUND)
        target_link_libraries(${target_name} PRIVATE OpenMP::OpenMP_CXX)
    endif()

    set_target_properties(${target_name} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            BUILD_WITH_INSTALL_RPATH TRUE
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Platform-specific settings remain the same...
    if(APPLE)
        set_target_properties(${target_name} PROPERTIES
                INSTALL_RPATH "@loader_path;@executable_path;${Python_RUNTIME_LIBRARY_DIRS}"
        )
    elseif(UNIX AND NOT APPLE)
        set_target_properties(${target_name} PROPERTIES
                INSTALL_RPATH "$ORIGIN:$ORIGIN/lib:${Python_RUNTIME_LIBRARY_DIRS}"
        )
        target_link_libraries(${target_name} PRIVATE X11)
    elseif(MSVC)
        target_compile_definitions(${target_name} PRIVATE
                UNICODE
                _UNICODE
        )
        set_property(TARGET ${target_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    endif()

    # Copy EngineDLL after build
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:EngineDLL>"
            "$<TARGET_FILE_DIR:${target_name}>"
    )
endfunction()

# Set up both targets
setup_test_target(Tests)
setup_test_target(VisualizationTest)

# Make the VisualizationTest target visible to CLion
set_target_properties(VisualizationTest PROPERTIES
        FOLDER "Tests"
)