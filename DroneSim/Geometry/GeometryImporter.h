#pragma once
#include "Geometry.h"
#include <string.h>
#include "MeshPrimitives.h"

namespace drosim::tools {

    bool loadObj(const char* path, drosim::tools::scene_data* data);

    bool parseObj(FILE file);

    void prepareGeometry(drosim::tools::scene scene, drosim::tools::scene_data scene_data);
}