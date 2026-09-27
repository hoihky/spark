#include "spark/gameplay/CameraBounds2DResolve.hpp"

#include "spark/ecs/components/camera/CameraBounds2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

bool TryResolveCameraBounds2DForTarget(
        const GameWorld& world,
        const GameObject& followTarget,
        Vector2& boundsMinOut,
        Vector2& boundsMaxOut) noexcept {
    const Vector3 targetPos = followTarget.GetWorldMatrix().TranslationVector();
    const float targetX = targetPos.x;
    const float targetY = targetPos.y;

    const CameraBounds2DComponent* best = nullptr;
    const GameObject* bestOwner = nullptr;
    int bestPriority = -1;

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        const CameraBounds2DComponent* bounds = object->GetComponent<CameraBounds2DComponent>();
        if (bounds == nullptr || !bounds->IsEnabled()) {
            return;
        }
        if (!bounds->ContainsWorldPoint(*object, targetX, targetY)) {
            return;
        }
        if (bounds->GetPriority() < bestPriority) {
            return;
        }
        bestPriority = bounds->GetPriority();
        best = bounds;
        bestOwner = object;
    });

    if (best == nullptr || bestOwner == nullptr) {
        return false;
    }

    CollisionAabb2 aabb{};
    best->ComputeWorldBounds(*bestOwner, aabb);
    boundsMinOut = {aabb.minX, aabb.minY};
    boundsMaxOut = {aabb.maxX, aabb.maxY};
    return true;
}

}  // namespace Spark
