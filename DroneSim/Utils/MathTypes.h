#pragma once

#include "../Common/CommonHeaders.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace drosim::math {
	constexpr float pi = 3.1415926535897932384626433832795f;
	constexpr float epsilon = 1e-5f;

	// Vector types
	using v2 = glm::vec2;
	using v2a = glm::vec2; // GLM handles alignment internally
	using v3 = glm::vec3;
	using v3a = glm::vec3; // GLM handles alignment internally
	using v4 = glm::vec4;
	using v4a = glm::vec4; // GLM handles alignment internally

	// Integer vector types
	using u32v2 = glm::uvec2;
	using u32v3 = glm::uvec3;
	using u32v4 = glm::uvec4;
	using s32v2 = glm::ivec2;
	using s32v3 = glm::ivec3;
	using s32v4 = glm::ivec4;

	// Matrix types
	using m3x3 = glm::mat3x3;
	using m4x4 = glm::mat4x4;
	using m4x4a = glm::mat4x4; // GLM handles alignment internally

	template<u32 bits>
	constexpr u32 pack_unit_float(f32 f)
	{
		static_assert(bits <= sizeof(u32) * 8);
		assert(f >= 0.f && f <= 1.f);
		constexpr f32 intervals{(f32)((1u << bits) - 1)};  // Changed 1ui32 to 1u
		return (u32)(intervals * f + 0.5f);
	}

	template<u32 bits>
	constexpr f32 unpack_unit_float(u32 i) {
		static_assert(bits <= sizeof(u32) * 8);
		assert(i < (1u << bits));  // Changed 1ui32 to 1u
		constexpr f32 intervals{ (f32)((1u << bits) - 1)};
		return (f32)i / intervals;
	}

	template<u32 bits>
	constexpr u32 pack_float(f32 f, f32 min, f32 max) {
		assert(min < max);
		assert(f <= max && f >= min);
		const f32 distance{(f-min)/(max - min)};
		return pack_unit_float<bits>(distance);
	}

	template<u32 bits>
	constexpr f32 unpack_float(u32 i, f32 min, f32 max) {
		assert(min < max);
		return unpack_unit_float<bits>(i) * (max -min) + min;
	}
}