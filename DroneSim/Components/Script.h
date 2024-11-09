#pragma once
#include "ComponentCommon.h"

namespace drosim::script {

	struct init_info{
		detail::script_creator script_creator;
	};

	component create(init_info info, game_entity::entity entity);
	void remove(component t);
	void shutdown();
}