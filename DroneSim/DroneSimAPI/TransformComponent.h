#pragma once
#include "../Components/ComponentCommon.h"

namespace drosim::transform {

	DEFINE_TYPED_ID(transform_id);

	class component final {
	public:
		constexpr explicit component(transform_id id) : _id(id) {};
		constexpr component() : _id(id::invalid_id) {};
		constexpr transform_id get_id() const { return _id; }
		constexpr bool is_valid() const { return id::is_valid(_id); }

		// Get current transform values
		math::v4 rotation() const;
		math::v3 scale() const;
		math::v3 position() const;

		// set transform values
		void set_rotation(const math::v4& rotation);
		void set_rotation_euler(const math::v3& euler_angles);
		void set_scale(const math::v3& scale);
		void set_position(const math::v3& position);

		// Transform operations
		void translate(const math::v3& translation);
		void rotate(const math::v3& euler_angles);
		void scale_by(const math::v3& scale_factor);

		// Get transformation matrices
		math::m4x4 get_transform_matrix() const;

		void reset();
	private:
		transform_id _id;
	};

}
