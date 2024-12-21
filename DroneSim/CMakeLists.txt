set(PROJECT_NAME DroneSim)

# Collect source files
file(GLOB_RECURSE SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Create static library
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Common
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
    target_compile_definitions(${PROJECT_NAME} PUBLIC USE_OPENMP)
endif()

# Link dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
    glad
    glm
    Python::Python
    pybind11::pybind11
)

# Platform-specific configurations
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MACOSX)
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX)
endif()

# Set properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)