#pragma once
#include "IPhysicsSystem.h"
#include <memory>
#include <vector>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace drosim::physics {

class PhysicsSystemManager {
public:
  PhysicsSystemManager() = default;

  // Template method to add any system derived from IPhysicsSystem
  template<typename T, typename... Args>
  T* AddSystem(Args&&... args) {
    static_assert(std::is_base_of<IPhysicsSystem, T>::value, "T must derive from IPhysicsSystem");
    auto system = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = system.get();
    systems.emplace_back(std::move(system));
    return ptr;
  }

  // Initialize all systems
  void Initialize() {
    for(auto& system : systems) {
      system->Initialize();
    }
  }

  // Update all systems
  void Update(float deltaTime) {
    for(auto& system : systems) {
      system->Update(deltaTime);
    }
  }

  // Retrieve a specific system by type
  template<typename T>
  T* GetSystem() {
    for(auto& system : systems) {
      if(auto ptr = dynamic_cast<T*>(system.get())) {
        return ptr;
      }
    }
    return nullptr;
  }

private:
  std::vector<std::unique_ptr<IPhysicsSystem>> systems;
};

}
