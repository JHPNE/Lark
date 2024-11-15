#include "Transform.h"
#include "Entity.h"

namespace drosim::transform {

	namespace {
		util::vector<math::v3> positions;
		util::vector<math::v4> rotations; // Using vec4 for quaternions
		util::vector<math::v3> scales;
	}

	component create(init_info info, game_entity::entity entity) {
		assert(entity.is_valid());
		const id::id_type entity_index{ id::index(entity.get_id()) };

		if (positions.size() > entity_index) {
			// Convert arrays to GLM vectors
			rotations[entity_index] = math::v4(info.rotation[0], info.rotation[1],
											 info.rotation[2], info.rotation[3]);
			positions[entity_index] = math::v3(info.position[0], info.position[1],
											 info.position[2]);
			scales[entity_index] = math::v3(info.scale[0], info.scale[1],
										  info.scale[2]);
		}
		else {
			assert(positions.size() == entity_index);
			rotations.emplace_back(math::v4(info.rotation[0], info.rotation[1],
										  info.rotation[2], info.rotation[3]));
			positions.emplace_back(math::v3(info.position[0], info.position[1],
										  info.position[2]));
			scales.emplace_back(math::v3(info.scale[0], info.scale[1],
									   info.scale[2]));
		}
		return component(transform_id{ (id::id_type)positions.size() - 1 });
	}

	void remove(component t) {
		assert(t.is_valid());
	}

	math::v4 component::rotation() const {
		assert(is_valid());
		return rotations[id::index(_id)];
	}

	math::v3 component::scale() const {
		assert(is_valid());
		return scales[id::index(_id)];
	}

	math::v3 component::position() const {
		assert(is_valid());
		return positions[id::index(_id)];
	}
}