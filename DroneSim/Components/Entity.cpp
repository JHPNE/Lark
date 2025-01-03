#include "Entity.h"
#include "Transform.h"
#include "Script.h"
#include "Geometry.h"

namespace lark::game_entity {
	// private alternative to static
	namespace {
		util::vector<transform::component> transforms;
		util::vector<script::component> scripts;
		util::vector<geometry::component> geometries;

		std::vector<id::generation_type> generations;
		util::deque<entity_id> free_ids;

		util::vector<entity_id> active_entities;
	}

	entity create(entity_info info) {
		assert(info.transform); // transform is required
		if (!info.transform) return {};

		entity_id id;
		if (free_ids.size() > id::min_deleted_elements) {
			id = free_ids.front();
			assert(!is_alive(id));
			free_ids.pop_front();
			id = entity_id{ id::new_generation(id) };
			++generations[id::index(id)];
		}
		else {
			id = entity_id{(id::id_type) generations.size()};
			generations.push_back(0);

			// Resize Components
			// emplace isntead of resize for memory allocations
			transforms.emplace_back();
			scripts.emplace_back();
			geometries.emplace_back();
		}

		const entity new_entity{ id };
		const id::id_type index{ id::index(id) };

		// Create Transform Component
		assert(!transforms[index].is_valid());
		transforms[index] = transform::create(*info.transform, new_entity);
		if (!transforms[index].is_valid()) return {};

		// Create Script Component
		if (info.script && info.script->script_creator) {
			assert(!scripts[index].is_valid());
			scripts[index] = script::create(*info.script, new_entity);
			assert(scripts[index].is_valid());
		}

		// Create Geometry Component
		if (info.geometry && info.geometry->scene) {
			assert(!geometries[index].is_valid());
			geometries[index] = geometry::create(*info.geometry, new_entity);
		}

		if (new_entity.is_valid()) {
			active_entities.push_back(new_entity.get_id());
		}

		return new_entity;
	}

	void remove(entity_id id) {
		const id::id_type index{ id::index(id) };
		assert(is_alive(id));

		// First invalidate any script component
		if (scripts[index].is_valid()) {
			auto script_copy = scripts[index]; // Make a copy before invalidating
			scripts[index] = {}; // Invalidate first
			script::remove(script_copy); // Then remove using the copy
		}

		if (geometries[index].is_valid()) {
			auto geometry_copy = geometries[index];
			geometries[index] = {};
			geometry::remove(geometry_copy);
		}

		transform::remove(transforms[index]);
		transforms[index] = {};

		if (generations[index] < id::max_generation)
		{
			free_ids.push_back(id);
		}

		auto it = std::find(active_entities.begin(), active_entities.end(), id);
		if (it != active_entities.end()) {
			util::erase_unordered(active_entities, it - active_entities.begin());
		}
	}

	const util::vector<entity_id>& get_active_entities() {
		return active_entities;
	}

	bool is_alive(entity_id id) {
		assert(id::is_valid(id));
		const id::id_type index{ id::index(id) };
		assert(index < generations.size());
		return (generations[index] == id::generation(id) && transforms[index].is_valid());
	}

	transform::component entity::transform() const {
		assert(is_alive(_id));
		const id::id_type index{ id::index(_id) };
		return transforms[index];
	}

	script::component entity::script() const {
		assert(is_alive(_id));
		return scripts[id::index(_id)];
	}

	geometry::component entity::geometry() const {
		assert(is_alive(_id));
		return geometries[id::index(_id)];
	}
}
