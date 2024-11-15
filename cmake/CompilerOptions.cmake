# Compiler-specific options
if(MSVC)
    add_compile_options(
            /W4
            /MP
            /permissive-
            /Zc:preprocessor
            /Zc:__cplusplus
            /wd4251
            $<$<CONFIG:Release,ReleaseEditor>:/O2>
            $<$<CONFIG:Debug,DebugEditor>:/Od>
    )

    add_definitions(
            -D_CRT_SECURE_NO_WARNINGS
            -DNOMINMAX
            -DWIN32_LEAN_AND_MEAN
    )
else()
    # Common flags for GCC/Clang
    add_compile_options(
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release,ReleaseEditor>:-O3>
            $<$<CONFIG:Debug,DebugEditor>:-O0>
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fext-numeric-literals)
    endif()
endif()

if(MINGW)
    add_compile_options(-fPIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Set default visibility for shared libraries
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

# Enable PIC by default for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Platform-specific definitions
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
endif()