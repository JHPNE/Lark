#pragma once
#include "EngineCoreAPI.h"

#ifdef __cplusplus
extern "C"
{
#endif

    ENGINE_API lark::script::detail::script_creator GetScriptCreator(const char *name);
    ENGINE_API const char **GetScriptNames(size_t *count);
    ENGINE_API bool RegisterScript(const char *script_name);

#ifdef __cplusplus
}
#endif
