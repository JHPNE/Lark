#pragma once
#include <memory>
#include <vector>
#include "EngineAPI.h"

class PhysicService
{
public:
    static PhysicService& Get()
    {
        static PhysicService instance;
        return instance;
    }

    bool SetWorldSettings(glm::vec3 gravity, wind windtype)
    {
       return ::SetWorldSettings(gravity, windtype);
    }
};