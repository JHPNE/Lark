#pragma once
#include "../DroneCommonHeaders.h"

namespace lark::battery {
struct init_info : public lark::drone_data::BatteryBody {};

drone_component create(init_info info, drone_entity::entity entity);

void remove(drone_component t);

void batteryCalculateCharge(drone_component t);
}