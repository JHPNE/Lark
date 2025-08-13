#include "Environment.h"
#include "../Components/Transform.h"
#include "../Components/Physics.h"

namespace lark::physics {
    Environment::Environment(const Config& config)
        : config(config),
          wind_profile(std::make_shared<wind::NoWind>()) {}

    Environment::~Environment() {
        reset();
    }

    
}