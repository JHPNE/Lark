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
private:
    PhysicService() = default;
    ~PhysicService() = default;
    PhysicService(const PhysicService&) = delete;
    PhysicService& operator=(const PhysicService&) = delete;
};