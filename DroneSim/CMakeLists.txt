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

# Link dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
        glm
        Python::Python
        pybind11::pybind11
)

# Set properties
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)
