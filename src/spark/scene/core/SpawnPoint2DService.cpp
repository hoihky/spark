#include "spark/scene/core/SpawnPoint2DService.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/world/SpawnPoint2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] float FacingFromTransform(const TransformComponent& transform) noexcept {
    const Quaternion rotation = transform.GetLocalTransform().rotation;
    const float siny = 2.0F * (rotation.w * rotation.z + rotation.x * rotation.y);
    const float cosy = 1.0F - 2.0F * (rotation.y * rotation.y + rotation.z * rotation.z);
    return std::atan2(siny, cosy);
}

}  // namespace

SceneSpawnPose2D SpawnPoint2DService::Resolve(const GameWorld& world, const char* spawnName) const noexcept {
    SceneSpawnPose2D out{};
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (out.found || object == nullptr) {
            return;
        }
        const SpawnPoint2DComponent* spawnPoint = object->GetComponent<SpawnPoint2DComponent>();
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
        out.teamId = spawnPoint->GetTeamId();
        if (spawnPoint->GetUseTransformFacing()) {
            if (const TransformComponent* transform = object->GetComponent<TransformComponent>()) {
                out.facingRadians = FacingFromTransform(*transform);
            }
        } else {
            out.facingRadians = spawnPoint->GetFacingRadians();
        }
    });
    return out;
}

}  // namespace Spark
