#pragma once

#include "spark/physics/colliders/ColliderBakeContext2D.hpp"

namespace Spark {

class GameObject;

/**
 * Strategy for baking one kind of ECS collider into <c>Collider2D</c> snapshots (Open/Closed).
 * Register instances on <c>ColliderBakePipeline2D</c>.
 */
class IColliderBakeStrategy2D {
public:
    virtual ~IColliderBakeStrategy2D() = default;

    [[nodiscard]] virtual bool Contributes(GameObject& object) const noexcept = 0;
    virtual void Bake(GameObject& object, ColliderBakeContext2D& context) const = 0;
};

}  // namespace Spark
