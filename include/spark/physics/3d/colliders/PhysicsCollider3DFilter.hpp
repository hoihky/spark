#pragma once

#include "spark/physics/core/ColliderFilter.hpp"

namespace Spark {

class GameObject;

/** Resolves collision filter for a 3D object (per-collider fields, else <c>PhysicsLayer3DComponent</c>). */
class PhysicsCollider3DFilter final {
public:
    [[nodiscard]] static ColliderFilter Resolve(const GameObject& object) noexcept;
};

}  // namespace Spark
