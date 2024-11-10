#include "Common.h"
#include "CommonHeaders.h"
#include "..\DroneSim\Components\Script.h"
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#endif
#include <Windows.h>

using namespace drosim;

namespace {
    HMODULE game_code_dll{ nullptr };
    using get_script_creator_func = drosim::script::detail::script_creator(*)(size_t);
    get_script_creator_func get_script_creator{ nullptr };
    using get_script_names_func = LPSAFEARRAY(*)(void);
    get_script_names_func get_script_names{ nullptr };
}

EDITOR_INTERFACE u32 LoadGameCodeDll(const char* dll_path)
{
    if (game_code_dll) return FALSE;
    game_code_dll = LoadLibraryA(dll_path);
    assert(game_code_dll);
    get_script_names = (get_script_names_func)GetProcAddress(game_code_dll, "get_script_names");
    get_script_creator = (get_script_creator_func)GetProcAddress(game_code_dll, "get_script_creator");
    return (game_code_dll && get_script_creator && get_script_names) ? TRUE : FALSE;
}

EDITOR_INTERFACE u32 UnloadGameCodeDll()
{
    if (!game_code_dll) return FALSE;

    // Clean up scripts and entities before unloading
    engine::cleanup_engine_systems();

    return TRUE;
}

game_entity::entity entity_from_id(id::id_type id) {
	return game_entity::entity{ game_entity::entity_id{id} };
}

EDITOR_INTERFACE script::detail::script_creator GetScriptCreator(const char* name)
{
    return (game_code_dll && get_script_creator) ? get_script_creator(script::detail::string_hash()(name)) : nullptr;
}

EDITOR_INTERFACE LPSAFEARRAY GetScriptNames()
{
    return (game_code_dll && get_script_names) ? get_script_names() : nullptr;
}