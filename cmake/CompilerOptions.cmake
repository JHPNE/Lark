# cmake/CompilerOptions.cmake
# Compiler-specific options
if(MSVC)
    # MSVC-specific options
    add_compile_options(
            /W4
            /MP
            /permissive-
            /Zc:preprocessor
            /Zc:__cplusplus
            $<$<CONFIG:Release,ReleaseEditor>:/O2>
            $<$<CONFIG:Debug,DebugEditor>:/Od>
    )

    add_definitions(
            -D_CRT_SECURE_NO_WARNINGS
            -DNOMINMAX
            -DWIN32_LEAN_AND_MEAN
    )

else()
    # GCC/Clang options
    add_compile_options(
            -Wall
            -Wextra
            -Wpedantic
            -fext-numeric-literals  # Enable extended numeric literals
            $<$<CONFIG:Release,ReleaseEditor>:-O3>
            $<$<CONFIG:Debug,DebugEditor>:-O0>
    )
endif()

if(MINGW)
    add_compile_options(
            -Wall
            -Wextra
            -Wpedantic
            -fext-numeric-literals
            $<$<CONFIG:Release,ReleaseEditor>:-O3>
            $<$<CONFIG:Debug,DebugEditor>:-O0>
            -fPIC  # Add position independent code flag
    )

    # Add linker flags for MinGW
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Platform-specific definitions
if(PLATFORM_WINDOWS)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(PLATFORM_MACOS)
    add_definitions(-DPLATFORM_MACOS)
elseif(PLATFORM_LINUX)
    add_definitions(-DPLATFORM_LINUX)
endif()