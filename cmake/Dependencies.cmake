# cmake/Dependencies.cmake
# Third-party dependencies configuration
include(FetchContent)

# OpenMP/LLVM Installation Check
function(ensure_openmp)
    if(APPLE)
        if(NOT DEFINED CMAKE_C_COMPILER OR NOT DEFINED CMAKE_CXX_COMPILER)
            find_program(HOMEBREW_EXECUTABLE brew)
            if(HOMEBREW_EXECUTABLE)
                execute_process(
                        COMMAND ${HOMEBREW_EXECUTABLE} --prefix llvm
                        OUTPUT_VARIABLE LLVM_PREFIX
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        RESULT_VARIABLE LLVM_FOUND
                )

                if(NOT LLVM_FOUND EQUAL 0)
                    execute_process(
                            COMMAND ${HOMEBREW_EXECUTABLE} install llvm libomp
                            RESULT_VARIABLE LLVM_INSTALL_RESULT
                    )

                    if(NOT LLVM_INSTALL_RESULT EQUAL 0)
                        message(FATAL_ERROR "Failed to install LLVM/OpenMP")
                    endif()

                    execute_process(
                            COMMAND ${HOMEBREW_EXECUTABLE} --prefix llvm
                            OUTPUT_VARIABLE LLVM_PREFIX
                            OUTPUT_STRIP_TRAILING_WHITESPACE
                    )
                endif()

                # Force CMake to use the Homebrew LLVM
                set(CMAKE_C_COMPILER "${LLVM_PREFIX}/bin/clang" CACHE STRING "C compiler" FORCE)
                set(CMAKE_CXX_COMPILER "${LLVM_PREFIX}/bin/clang++" CACHE STRING "C++ compiler" FORCE)

                # Set additional flags
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp" CACHE STRING "C flags" FORCE)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp" CACHE STRING "C++ flags" FORCE)

                # Set linker flags for OpenMP
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${LLVM_PREFIX}/lib -Wl,-rpath,${LLVM_PREFIX}/lib" CACHE STRING "Executable linker flags" FORCE)
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L${LLVM_PREFIX}/lib -Wl,-rpath,${LLVM_PREFIX}/lib" CACHE STRING "Shared library linker flags" FORCE)

                message(STATUS "Using Homebrew LLVM for OpenMP support")
                message(STATUS "LLVM installation: ${LLVM_PREFIX}")
                message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
                message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
            else()
                message(FATAL_ERROR "Homebrew not found. Cannot install LLVM/OpenMP")
            endif()
        endif()
    endif()
endfunction()

# Call the function to ensure OpenMP is available
ensure_openmp()

###############################################################################
# PyBind
###############################################################################
FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# OpenMP Configuration
if(APPLE)
    find_program(HOMEBREW_EXECUTABLE brew)
    if(HOMEBREW_EXECUTABLE)
        execute_process(
                COMMAND ${HOMEBREW_EXECUTABLE} --prefix llvm
                OUTPUT_VARIABLE LLVM_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE LLVM_FOUND
        )

        if(LLVM_FOUND EQUAL 0)
            # Set the compiler to Homebrew's LLVM before project() is called
            set(CMAKE_C_COMPILER "${LLVM_PREFIX}/bin/clang")
            set(CMAKE_CXX_COMPILER "${LLVM_PREFIX}/bin/clang++")

            # Add OpenMP flags
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

            # Add OpenMP library path
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${LLVM_PREFIX}/lib -Wl,-rpath,${LLVM_PREFIX}/lib")

            message(STATUS "Using Homebrew LLVM for OpenMP support")
            message(STATUS "LLVM installation: ${LLVM_PREFIX}")
            message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
            message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
        else()
            message(FATAL_ERROR "LLVM not found. Please install it using: brew install llvm")
        endif()
    else()
        message(FATAL_ERROR "Homebrew not found. Cannot configure LLVM/OpenMP.")
    endif()
endif()


# GLAD
FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v0.1.36
)

FetchContent_MakeAvailable(glad)

# GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Bullet Physics Configuration
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)  # Changed to OFF for static linking
set(USE_DOUBLE_PRECISION ON CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

# Fetch and include Bullet
FetchContent_Declare(
        bullet
        GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
        GIT_TAG 3.25    # Specify a stable version
)
FetchContent_MakeAvailable(bullet)

# ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
)

# TinyXML2
FetchContent_Declare(
        tinyxml2
        GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
        GIT_TAG 9.0.0
)
set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "" FORCE)

# GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8
)

FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_TAG master  # You might want to pin to a specific commit/tag for stability
)

# Make dependencies available
FetchContent_MakeAvailable(glfw imgui tinyxml2 glm imguizmo)


if(TARGET glad)
    set_target_properties(glad PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Ensure include directories are properly set
    target_include_directories(glad PUBLIC
            $<BUILD_INTERFACE:${glad_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
endif()


# Configure ImGui
add_library(imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
        "${imguizmo_SOURCE_DIR}/ImGuizmo.cpp"  # Add ImGuizmo source
)

target_include_directories(imgui PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
        "${imguizmo_SOURCE_DIR}"  # Add ImGuizmo include directory
)

target_include_directories(imgui PRIVATE
        ${glad_SOURCE_DIR}/include
        ${glfw_SOURCE_DIR}/include
)

target_link_libraries(imgui PUBLIC glad glfw)
