set(PROJECT_NAME EngineDLL)

# Explicitly list source files
set(SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/dllmain.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/EngineAPI.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/EntityAPI.cpp"
)

set(HEADER_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Common.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/EngineAPI.h"
)

# Create shared library
add_library(${PROJECT_NAME} SHARED
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

# Include directories
target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}  # Make the EngineDLL root directory public
        PRIVATE
        ${CMAKE_SOURCE_DIR}/DroneSim
        ${CMAKE_SOURCE_DIR}/DroneSim/Common
        ${CMAKE_SOURCE_DIR}/DroneSim/Components
        ${CMAKE_SOURCE_DIR}/DroneSim/DroneSimAPI
)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
        DroneSim
        glm
)

# Set properties
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ENGINEDLL_EXPORTS
            _USRDLL
            UNICODE
            _UNICODE
    )
    # Windows-specific output naming
    set_target_properties(${PROJECT_NAME} PROPERTIES
            PREFIX ""
            SUFFIX ".dll"
    )
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ENGINEDLL_EXPORTS
    )
    # macOS-specific output naming
    set_target_properties(${PROJECT_NAME} PROPERTIES
            PREFIX "lib"
            SUFFIX ".dylib"
            INSTALL_RPATH "@loader_path"
            BUILD_WITH_INSTALL_RPATH TRUE
            MACOSX_RPATH ON
    )
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ENGINEDLL_EXPORTS
    )
    # Linux-specific output naming
    set_target_properties(${PROJECT_NAME} PROPERTIES
            PREFIX "lib"
            SUFFIX ".so"
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()


