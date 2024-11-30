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

    scene* get_scene() const;
    mesh* get_mesh() const;
    lod_group* get_lod_group() const;

  private:
    geometry_id _id;
  };

}
