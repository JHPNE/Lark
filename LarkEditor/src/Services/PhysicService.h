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

    bool setWorldSettings(wind windtype)
    {
        return SetWorldSettings(windtype);
    }

    bool setWind(wind_type windType, glm::vec3 windVector, glm::vec3 windAmplitudes, glm::vec3 windFrequencies)
    {
        return SetWind(windType, windVector, windAmplitudes, windFrequencies);
    }

private:
    PhysicService() = default;
    ~PhysicService() = default;
    PhysicService(const PhysicService&) = delete;
    PhysicService& operator=(const PhysicService&) = delete;
};