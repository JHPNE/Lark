#include <PxPhysicsAPI.h>
// Note: This include should work now since we're linking against the correct PhysX targets

namespace {
using namespace physx;

PxFoundation*        gFoundation = nullptr;
PxPhysics*           gPhysics    = nullptr;
PxScene*             gScene      = nullptr;
PxDefaultAllocator   gAllocatorCallback;
PxDefaultErrorCallback gErrorCallback;

PxDefaultCpuDispatcher* gDispatcher = nullptr;

} // anonymous