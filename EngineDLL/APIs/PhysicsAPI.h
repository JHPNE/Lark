#pragma once
#include "EngineCoreAPI.h"

#ifdef __cplusplus
extern "C"
{
#endif
    ENGINE_API bool SetWorldSettings(glm::vec3 gravity, wind windtype);
#ifdef __cplusplus
}
#endif
