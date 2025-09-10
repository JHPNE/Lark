#include "ScriptAPI.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C" {
    // Create a concrete script class for each script
    class PythonScriptWrapper : public lark::script::entity_script {
    public:
        explicit PythonScriptWrapper(lark::game_entity::entity entity)
            : entity_script(entity) {}
    };

    ENGINE_API script::detail::script_creator GetScriptCreator(const char* name) {
        if (!name) return nullptr;
        // Use engine's existing script registry system
        return script::detail::get_script_creator(
            script::detail::string_hash()(name));
    }

    ENGINE_API const char** GetScriptNames(size_t* count) {
        // First get the count of script names
        size_t name_count = 0;
        script::detail::get_script_names(nullptr, &name_count);

        if (name_count == 0) {
            *count = 0;
            return nullptr;
        }

        // Resize the static vector to hold all name pointers
        static std::vector<const char*> name_ptrs;
        name_ptrs.resize(name_count);

        // Now get the actual names
        script::detail::get_script_names(name_ptrs.data(), &name_count);
        *count = name_count;

        return name_ptrs.data();
    }

    ENGINE_API bool RegisterScript(const char* script_name) {
        if (!script_name) return false;

        size_t tag = lark::script::detail::string_hash()(script_name);
        if (lark::script::detail::script_exists(tag)) {
            return false;  // Script already registered
        }

        auto creator = [](lark::game_entity::entity entity) -> lark::script::detail::script_ptr {
            return std::make_unique<PythonScriptWrapper>(entity);
        };

        bool success = lark::script::detail::register_script(
            lark::script::detail::string_hash()(script_name),
            creator);

        if (success) {
            lark::script::detail::add_script_name(script_name);
        }

        return success;
    }

}