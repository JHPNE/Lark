# EngineDLL/CMakeLists.txt
set(PROJECT_NAME EngineDLL)

# Collect source files
file(GLOB_RECURSE SOURCE_FILES
        "*.cpp"
        "*.h"
)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/DroneSim/Common
)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
        DroneSim
)

# Set properties
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ENGINEDLL_EXPORTS
            _USRDLL
            UNICODE
            _UNICODE
    )
endif()
