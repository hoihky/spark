#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/Collision2D.hpp"

namespace Spark {

class GameObject;
class TilemapCollider2DComponent;
class TilemapComponent;
class SpatialHashGrid2D;

/** True when the object has a tilemap collider + tilemap and is not a dynamic rigidbody. */
[[nodiscard]] bool ContributesTilemapCollider2DStatic(GameObject& object) noexcept;

/**
 * Appends one <c>StaticCollider2D</c> box per solid tile and inserts each into the spatial hash.
 * Payload indices continue from <c>outStatics.GetSize()</c> on entry.
 */
void AppendTilemapCollider2DStatics(
        GameObject& owner,
        const TilemapCollider2DComponent& collider,
        const TilemapComponent& tilemap,
        Array<StaticCollider2D>& outStatics,
        SpatialHashGrid2D& outGrid);

}  // namespace Spark
