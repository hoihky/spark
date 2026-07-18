#pragma once

#include "spark/physics/Collision3D.hpp"

namespace Spark {

class GameObject;
class MeshCollider3DComponent;

void ComputeMeshCollider3WorldAabb(
        GameObject& owner,
        const MeshCollider3DComponent& collider,
        CollisionAabb3& outWorld) noexcept;

[[nodiscard]] bool ContributesMeshCollider3DStatic(const GameObject& object) noexcept;

}  // namespace Spark
