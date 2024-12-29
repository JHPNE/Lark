#pragma once
#include <btBulletCollisionCommon.h>

namespace drosim::physics {


class PhysicsTests {
public:
    void runTests(bool gpu) {
        collisionTest(gpu);
    }

    void collisionTest(bool gpu) {
        btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
    }

};
};;