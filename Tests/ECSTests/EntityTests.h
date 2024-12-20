#pragma once
#include <CommonHeaders.h>
#include "Components/Entity.h"
#include <Id.h>
#include <Components/Script.h>
#include <Components/Transform.h>
#include <Components/Geometry.h>
#include "Structures.h"

using namespace drosim;

class EntityTests {
  public:
    void runTests() {
      creationTest();
    };

    void creationTest(){
      size_t count = 50;

      for (int i = 0; i < count; i++) {
        transform::init_info transform_info{};
        transform_info.position[0] = transform_info.position[1] = transform_info.position[2] = 0;
        transform_info.scale[0] = transform_info.scale[1] = transform_info.scale[2] = 1;
        transform_info.rotation[0] = 0;
        transform_info.rotation[1] = 0;
        transform_info.rotation[2] = 0;
        transform_info.rotation[3] = 0;

        game_entity::entity_info entity_info{
          &transform_info,
        };

        auto entity = game_entity::create(entity_info);
      }
      printf("Succesfully created %d Entities \n", count);

    };
};
