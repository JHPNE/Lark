#include "Script.h"
#include "Entity.h"


namespace drosim::script {
	namespace {
		util::vector<detail::script_ptr> entity_scripts;
		util::vector<id::id_type> id_mapping;

		util::vector<id::generation_type> generations;
		util::deque<script_id> free_ids;

		using script_registry = std::unordered_map<size_t, detail::script_creator>;
		
		script_registry& registry() {
			// prevents access to registry before it is initialized with data
			static script_registry reg;
			return reg;
		}



		bool exists(script_id id)
		{
			assert(id::is_valid(id));
			const id::id_type index{ id::index(id) };
			assert(index < generations.size() && !(id::is_valid(id_mapping[index]) && id_mapping[index] >= entity_scripts.size()));
			assert(generations[index] == id::generation(id));
			return (id::is_valid(id_mapping[index]) &&
				generations[index] == id::generation(id)) &&
				entity_scripts[id_mapping[index]] &&
				entity_scripts[id_mapping[index]]->is_valid();
		}


	} // namespace

	namespace detail {
		u8 register_script(size_t tag, script_creator func) {
			bool result{ registry().insert(script_registry::value_type{ tag, func }).second };
			assert(result);
			return result;
		}
		script_creator get_script_creator(size_t tag) {
			auto script = drosim::script::registry().find(tag);
			assert(script != drosim::script::registry().end() && script->first == tag);
			return script->second;
		}


		util::vector<std::string>& script_name() {
			static util::vector<std::string> names;
			return names;
		}

		u8 add_script_name(const char* name) {
			script_name().emplace_back(name);
			return true;
		}

		void get_script_names(const char** names, size_t* count) {
			auto& script_names = drosim::script::detail::script_name();
			*count = script_names.size();

			if (!names) return;

			if (*count > 0) {
				for (size_t i = 0; i < *count; ++i) {
					names[i] = script_names[i].c_str();
				}
			}
		}

	} // namespace detail

	void shutdown() {
		// Clear all script data
		entity_scripts.clear();
		id_mapping.clear();
		generations.clear();
		free_ids.clear();
		registry().clear();
		detail::script_name().clear();

	}

	component create(init_info info, game_entity::entity entity) {
		assert(entity.is_valid());
		assert(info.script_creator);
		script_id id{};

		if (free_ids.size() > id::min_deleted_elements) {
			id = free_ids.front();
			assert(!exists(id));
			free_ids.pop_front();
			id = script_id{ id::new_generation(id) };
			++generations[id::index(id)];
		}
		else {
			id = script_id{ (id::id_type)id_mapping.size() };
			id_mapping.emplace_back();
			generations.push_back(0);
		}

		assert(id::is_valid(id));	
		const id::id_type index{ (id::id_type) entity_scripts.size()};
		entity_scripts.emplace_back(info.script_creator(entity));
		assert(entity_scripts.back()->get_id() == entity.get_id());
		id_mapping[id::index(id)] = index;	

		return component{id};
	}

	void remove(component c) {
		// Change the validation to be more defensive
		if (!c.is_valid()) return;
		if (!exists(c.get_id())) return;

		const script_id id{ c.get_id() };
		const id::id_type index{ id_mapping[id::index(id)]};
		const script_id last_id{ entity_scripts.back()->script().get_id()};
		util::erase_unordered(entity_scripts, index);
		id_mapping[id::index(last_id)] = index;
		id_mapping[id::index(id)] = id::invalid_id;

		if (generations[index] < id::max_generation)
		{
			free_ids.push_back(id);
		}
	}

}
