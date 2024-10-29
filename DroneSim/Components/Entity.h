#pragma once

#include "ComponentCommon.h"

namespace drosim {

	#define INIT_INFO(component) namespace component { struct init_info; }
	INIT_INFO(transform);
	#undef INIT_INFO


	namespace game_entity {
		struct entityInfo {
			transform::init_info* transform{ nullptr };
		};

		entity create_game_entity(const entityInfo& info);
		void remove_game_entity(entity id);
		bool is_alive(entity id);
	}

}

