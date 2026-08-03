#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/colliders/Collider2D.hpp"

namespace Spark {

class GameObject;
class PolygonCollider2DComponent;
class SpatialHashGrid2D;

/** True when the object has a polygon collider with >= 3 vertices and is not dynamic. */
[[nodiscard]] bool ContributesPolygonCollider2DStatic(GameObject& object) noexcept;

void AppendPolygonCollider2DStatic(
        GameObject& owner,
        const PolygonCollider2DComponent& collider,
        Array<Collider2D>& outColliders,
        SpatialHashGrid2D& outGrid);

}  // namespace Spark
