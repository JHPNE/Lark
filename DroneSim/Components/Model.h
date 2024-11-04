#pragma once

#include "..\Common\CommonHeaders.h"

namespace drosim{
	struct vertex {
		math::v3 position;
		math::v3 normal;
		math::v2 texcoord;
	};

	struct mesh {
		util::vector<vertex> vertices;
		util::vector<u32> indices;
	};

	struct model {
		util::vector<mesh> meshes;
	};
}
