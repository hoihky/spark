#include "spark/scene/core/SpawnPointService.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/world/SpawnPointComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cstring>

namespace Spark {

SceneSpawnPose SpawnPointService::Resolve(const GameWorld& world, const char* spawnName) const noexcept {
    SceneSpawnPose out{};
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (out.found || object == nullptr) {
            return;
        }
        const SpawnPointComponent* spawnPoint = object->GetComponent<SpawnPointComponent>();
        if (spawnPoint == nullptr) {
            return;
        }
        if (spawnName != nullptr && spawnName[0] != '\0' &&
            std::strcmp(spawnPoint->GetSpawnName().CStr(), spawnName) != 0) {
            return;
        }
        out.found = true;
        out.object = object;
        out.component = spawnPoint;
        out.position = object->GetWorldMatrix().TranslationVector();
        if (const TransformComponent* transform = object->GetComponent<TransformComponent>()) {
            out.rotation = transform->GetLocalTransform().rotation;
        }
    });
    return out;
}

}  // namespace Spark
