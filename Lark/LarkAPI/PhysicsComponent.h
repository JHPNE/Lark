#pragma once
#include "../Components/ComponentCommon.h"

namespace lark::physics {

    DEFINE_TYPED_ID(physics_id);

    class component final {
    public:
        constexpr explicit component(physics_id id) : _id{id} {}
        constexpr component() : _id{id::invalid_id} {}
        constexpr physics_id get_id() const { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

        // Physics operations
        void step(float dt);

    private:
        physics_id _id;
    };
}
