#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"
#include "spark/scene/tilemap/TileCell.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"

namespace Spark {

class GameObject;

/**
 * Bakes one map cell into <c>outColliders</c> using tile definition + per-cell transform.
 * @return false when the cell has no collision contribution.
 */
bool AppendTilemapCellCollider2D(
        GameObject& owner,
        const TilemapCollider2DComponent& collider,
        const Matrix4& worldMatrix,
        float tileWorldSize,
        std::uint32_t tileX,
        std::uint32_t tileY,
        const TileCell& cell,
        const TileDefinition& definition,
        Array<Collider2D>& outColliders,
        SpatialHashGrid2D& outGrid);

}  // namespace Spark
