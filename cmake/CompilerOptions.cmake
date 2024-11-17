# cmake/CompilerOptions.cmake

# Compiler-specific options
if(MSVC)
    add_compile_options(
            /W4             # Warning level 4
            /MP             # Multi-processor compilation
            /permissive-    # Standards conformance
            /Zc:preprocessor # Enable preprocessor conformance mode
            /Zc:__cplusplus # Enable proper __cplusplus macro
            /wd4251         # Disable warning about dll-interface
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Debug>:/Od>
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
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Debug>:-O0>
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