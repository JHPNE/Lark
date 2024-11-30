#pragma once
#include "ComponentCommon.h"

namespace drosim::geometry {

struct init_info {
  detail::geometry_creator geometry_creator;
};


component create(init_info info, game_entity::entity entity);

void remove(component t);
void shutdown();
}