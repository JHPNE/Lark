#pragma once
#include "Geometry.h"
#include <string.h>
#include "MeshPrimitives.h"

namespace lark::tools {

    bool loadObj(const char* path, lark::tools::scene_data* data);

    bool parseObj(FILE file);

    void prepareGeometry(lark::tools::scene scene, lark::tools::scene_data scene_data);
}