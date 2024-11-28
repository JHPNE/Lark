set(PROJECT_NAME EngineDLL)

# Explicitly list source files
set(SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/dllmain.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/EngineAPI.cpp"
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
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/DroneSim
        ${CMAKE_SOURCE_DIR}/DroneSim/Common
        ${CMAKE_SOURCE_DIR}/DroneSim/Components
        ${CMAKE_SOURCE_DIR}/DroneSim/DroneSimAPI
        ${CMAKE_SOURCE_DIR}/DroneSim/Scripting
        ${Python_INCLUDE_DIRS}
        ${pybind11_INCLUDE_DIRS}
)

# OpenMP configuration
if(OpenMP_CXX_FOUND)
    if(MINGW)
        target_compile_options(${PROJECT_NAME} PUBLIC ${OpenMP_FLAGS})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenMP_LIBS})
    else()
        target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
    endif()
endif()

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
        DroneSim
        glm
        Python::Python
        pybind11::pybind11
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


