#pragma once

#define USE_STL_VECTOR 1
#define USE_STL_DEQUE 1
#include <glm/vec3.hpp>

#include "LinearMath/btVector3.h"

#if USE_STL_VECTOR
	#include <vector>
	#include <algorithm>
namespace lark::util {
	template<typename T>
	using vector = std::vector<T>;

	template<typename T> void erase_unordered(std::vector<T>& v, size_t index) {
		if (v.size() > 1) {
			std::iter_swap(v.begin() + index, v.end() - 1);
			v.pop_back();
		}
		else
		{
			v.clear();
		}
	}


	inline btVector3 glm_to_bt_vector3(glm::vec3 vec) {
		return btVector3{vec.x, vec.y, vec.z};
	}

	inline glm::vec3 bt_to_glm_vec3(const btVector3& v) {
		return glm::vec3(v.x(), v.y(), v.z());
	}
}
#endif

#if USE_STL_DEQUE
#include <deque>
namespace lark::util {
	template<typename T>
	using deque = std::deque<T>;
}
#endif

namespace lark::util {
}
