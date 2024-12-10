#pragma once
#include "../Components/ComponentCommon.h"
#include "../Geometry/Geometry.h"

using namespace drosim::tools;

namespace drosim::geometry {

  DEFINE_TYPED_ID(geometry_id);

  class component final {
  public:
    constexpr explicit component(geometry_id id) : _id{ id } {}
    constexpr component() : _id{ id::invalid_id } {}
    constexpr geometry_id get_id() const { return _id; }
    constexpr bool is_valid() const { return id::is_valid(_id); }

    tools::scene* get_scene() const;
    tools::mesh* get_mesh() const;
    tools::lod_group* get_lod_group() const;

    /**
     * @brief Set whether this geometry component is dynamic
     * @param dynamic Whether to enable dynamic mode
     * @return true if the operation was successful
     */
    bool set_dynamic(bool dynamic);

    /**
     * @brief Update vertex positions for a dynamic geometry
     * @param new_positions New vertex positions
     * @return true if the operation was successful
     */
    bool update_vertices(const std::vector<math::v3>& new_positions);

    /**
     * @brief Check if this geometry is dynamic
     * @return true if the geometry is dynamic
     */
    bool is_dynamic() const;

  private:
    geometry_id _id;
  };

}
