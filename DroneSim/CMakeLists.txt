set(PROJECT_NAME DroneSim)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "Common/CommonHeaders.h"
    "Common/Id.h"
    "Common/PrimitiveTypes.h"
    "Components/ComponentCommon.h"
    "Components/Entity.cpp"
    "Components/Entity.h"
    "Components/Script.cpp"
    "Components/Script.h"
    "Components/Transform.cpp"
    "Components/Transform.h"
    "Core/Main.cpp"
    "DroneSimAPI/GameEntity.h"
    "DroneSimAPI/ScriptComponent.h"
    "DroneSimAPI/TransformComponent.h"
    "Utils/MathTypes.h"
    "Utils/Util.h"
)
source_group("" FILES ${no_group_source_files})

set(ALL_FILES
    ${no_group_source_files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE DroneSim)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE       "TRUE"
        INTERPROCEDURAL_OPTIMIZATION_RELEASEEDITOR "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE       "TRUE"
        INTERPROCEDURAL_OPTIMIZATION_RELEASEEDITOR "TRUE"
    )
endif()
################################################################################
# Include directories
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Common"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Common"
    )
endif()

################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:DebugEditor>:"
            "_DEBUG;"
            "USE_WITH_EDITOR"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "$<$<CONFIG:ReleaseEditor>:"
            "NDEBUG;"
            "USE_WITH_EDITOR"
        ">"
        "_LIB;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:DebugEditor>:"
            "_DEBUG;"
            "USE_WITH_EDITOR"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "$<$<CONFIG:ReleaseEditor>:"
            "NDEBUG;"
            "USE_WITH_EDITOR"
        ">"
        "_LIB;"
        "UNICODE;"
        "_UNICODE"
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /Oi;
                /Gy
            >
            $<$<CONFIG:ReleaseEditor>:
                /Oi;
                /Gy
            >
            /permissive-;
            /MP;
            /std:c++17;
            /sdl-;
            /Gd;
            /W4;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /fp:fast;
            /GR-;
            /GS-;
            /Y-
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /Oi;
                /Gy
            >
            $<$<CONFIG:ReleaseEditor>:
                /Oi;
                /Gy
            >
            /permissive-;
            /MP;
            /std:c++17;
            /sdl-;
            /Gd;
            /W4;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /fp:fast;
            /GR-;
            /GS-;
            /Y-
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
            $<$<CONFIG:ReleaseEditor>:
                /OPT:REF;
                /OPT:ICF
            >
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /OPT:ICF
            >
            $<$<CONFIG:ReleaseEditor>:
                /OPT:REF;
                /OPT:ICF
            >
        )
    endif()
endif()

