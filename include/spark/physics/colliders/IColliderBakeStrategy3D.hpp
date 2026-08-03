#pragma once

#include "spark/physics/colliders/ColliderBakeContext3D.hpp"

namespace Spark {

class GameObject;

/** Strategy for baking one kind of ECS collider into <c>Collider3D</c> snapshots. */
class IColliderBakeStrategy3D {
public:
    virtual ~IColliderBakeStrategy3D() = default;

    [[nodiscard]] virtual bool Contributes(GameObject& object) const noexcept = 0;
    virtual void Bake(GameObject& object, ColliderBakeContext3D& context) const = 0;
};

}  // namespace Spark
