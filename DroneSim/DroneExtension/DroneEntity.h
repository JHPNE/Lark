#pragma once
#include "../Common/CommonHeaders.h"
#include "Components/FuselageComponent.h"
#include "Components/BatteryComponent.h"

namespace lark {
  namespace drone_entity {
    DEFINE_TYPED_ID(drone_id);

    class entity {
      public:
        constexpr explicit entity(drone_id id) : _id{ id } {}
        constexpr entity() : _id{ id::invalid_id } {}
        constexpr drone_id get_id() const { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

        fuselage::drone_component fuselage() const;

      private:
        drone_id _id;
    };
  }

  namespace fuselage {
    class fuselage : public drone_entity::entity {
      public:
        virtual ~fuselage() = default;
    };
  }

  namespace battery {
    class battery : public drone_entity::entity {
      public:
        virtual ~battery() = default;
    };
  }
}

