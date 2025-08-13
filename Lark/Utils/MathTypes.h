#pragma once

#include "../Common/CommonHeaders.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lark::math {
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

	/**
	 * Solves Ax = b using Gaussian elimination with partial pivoting
	 * @param A NxN matrix (row-major)
	 * @param b Nx1 vector
	 * @return Solution vector x
	 */
	template<size_t N>
	static std::array<float, N> solve(std::array<std::array<float, N>, N> A, std::array<float, N> b) {
		// Forward elimination with partial pivoting
		for (size_t k = 0; k < N - 1; ++k) {
			// Find pivot
			size_t pivot_row = k;
			float max_val = std::abs(A[k][k]);
			for (size_t i = k + 1; i < N; ++i) {
				if (std::abs(A[i][k]) > max_val) {
					max_val = std::abs(A[i][k]);
					pivot_row = i;
				}
			}

			// Swap rows if needed
			if (pivot_row != k) {
				std::swap(A[k], A[pivot_row]);
				std::swap(b[k], b[pivot_row]);
			}

			// Check for singular matrix
			if (std::abs(A[k][k]) < 1e-10f) {
				printf("Singular matrix in trajectory computation");
			}

			// Eliminate column
			for (size_t i = k + 1; i < N; ++i) {
				float factor = A[i][k] / A[k][k];
				for (size_t j = k + 1; j < N; ++j) {
					A[i][j] -= factor * A[k][j];
				}
				b[i] -= factor * b[k];
				A[i][k] = 0.0f;
			}
		}

		// Back substitution
		std::array<float, N> x{};
		for (int i = N - 1; i >= 0; --i) {
			x[i] = b[i];
			for (size_t j = i + 1; j < N; ++j) {
				x[i] -= A[i][j] * x[j];
			}
			x[i] /= A[i][i];
		}

		return x;
	}

	/**
	 * Solves Ax = B where B has multiple columns
	 * @param A NxN matrix
	 * @param B NxM matrix (each column is a separate RHS)
	 * @return Solution matrix X (NxM)
	 */
	template<size_t N, size_t M>
	static std::array<std::array<float, M>, N> solveMultiple(
		const std::array<std::array<float, N>, N>& A,
		const std::array<std::array<float, M>, N>& B) {

		std::array<std::array<float, M>, N> X{};

		// Solve for each column separately
		for (size_t col = 0; col < M; ++col) {
			// Extract column from B
			std::array<float, N> b_col{};
			for (size_t row = 0; row < N; ++row) {
				b_col[row] = B[row][col];
			}

			// Solve system
			auto x_col = solve<N>(A, b_col);

			// Store result
			for (size_t row = 0; row < N; ++row) {
				X[row][col] = x_col[row];
			}
		}

		return X;
	}
}