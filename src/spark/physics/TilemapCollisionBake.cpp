#include "spark/physics/TilemapCollisionBake.hpp"

#include "spark/math/Vector4.hpp"
#include "spark/scene/tilemap/TileCellSpace.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Hp3(const Vector4& p) noexcept {
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

void EncapsulateWorldPoint(const Vector4& worldH, CollisionAabb2& aabb) noexcept {
    const Vector3 v = Hp3(worldH);
    aabb.minX = std::min(aabb.minX, v.x);
    aabb.maxX = std::max(aabb.maxX, v.x);
    aabb.minY = std::min(aabb.minY, v.y);
    aabb.maxY = std::max(aabb.maxY, v.y);
}

void LocalTilePointToWorld(
        const Matrix4& worldMatrix,
        const float tileWorldSize,
        const std::uint32_t tileX,
        const std::uint32_t tileY,
        const Vector2& normalized,
        Vector4& outWorldH) noexcept {
    const float lx = (static_cast<float>(tileX) + normalized.x) * tileWorldSize;
    const float ly = (static_cast<float>(tileY) + normalized.y) * tileWorldSize;
    outWorldH = worldMatrix * Vector4(lx, ly, 0.0F, 1.0F);
}

void BuildWorldAabbFromNormalizedCorners(
        const Matrix4& worldMatrix,
        const float tileWorldSize,
        const std::uint32_t tileX,
        const std::uint32_t tileY,
        const std::uint8_t transformFlags,
        const Vector2* normalizedCorners,
        const std::size_t cornerCount,
        CollisionAabb2& outAabb) noexcept {
    outAabb = {1.0e30F, 1.0e30F, -1.0e30F, -1.0e30F};
    for (std::size_t i = 0; i < cornerCount; ++i) {
        const Vector2 t = TransformNormalizedCellPoint(normalizedCorners[i], transformFlags);
        Vector4 wh{};
        LocalTilePointToWorld(worldMatrix, tileWorldSize, tileX, tileY, t, wh);
        EncapsulateWorldPoint(wh, outAabb);
    }
}

void AppendBoxCollider(
        GameObject& owner,
        const TilemapCollider2DComponent& collider,
        const CollisionAabb2& aabb,
        Array<StaticCollider2D>& outStatics,
        SpatialHashGrid2D& outGrid) {
    StaticCollider2D entry{};
    entry.shape = StaticCollider2DShape::Box;
    entry.categoryBits = collider.GetCategoryBits();
    entry.maskBits = collider.GetMaskBits();
    entry.owner = &owner;
    entry.isTrigger = collider.GetIsTrigger();
    entry.aabb = aabb;
    const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
    outStatics.PushBack(entry);
    outGrid.InsertIndexedAabb(idx, entry.aabb);
}

void AppendConvexCollider(
        GameObject& owner,
        const TilemapCollider2DComponent& collider,
        const Array<Vector2>& worldVerts,
        Array<StaticCollider2D>& outStatics,
        SpatialHashGrid2D& outGrid) {
    if (worldVerts.GetSize() < 3 || worldVerts.GetSize() > kMaxStaticPolygonVertices) {
        return;
    }
    StaticCollider2D entry{};
    entry.shape = StaticCollider2DShape::ConvexPolygon;
    entry.categoryBits = collider.GetCategoryBits();
    entry.maskBits = collider.GetMaskBits();
    entry.owner = &owner;
    entry.isTrigger = collider.GetIsTrigger();
    entry.polygonVertexCount = static_cast<std::uint8_t>(worldVerts.GetSize());
    entry.aabb = {1.0e30F, 1.0e30F, -1.0e30F, -1.0e30F};
    for (std::size_t i = 0; i < worldVerts.GetSize(); ++i) {
        entry.polygonVertsX[i] = worldVerts[i].x;
        entry.polygonVertsY[i] = worldVerts[i].y;
        entry.aabb.minX = std::min(entry.aabb.minX, worldVerts[i].x);
        entry.aabb.maxX = std::max(entry.aabb.maxX, worldVerts[i].x);
        entry.aabb.minY = std::min(entry.aabb.minY, worldVerts[i].y);
        entry.aabb.maxY = std::max(entry.aabb.maxY, worldVerts[i].y);
    }
    const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
    outStatics.PushBack(entry);
    outGrid.InsertIndexedAabb(idx, entry.aabb);
}

}  // namespace

bool AppendTilemapCellCollider2D(
        GameObject& owner,
        const TilemapCollider2DComponent& collider,
        const Matrix4& worldMatrix,
        const float tileWorldSize,
        const std::uint32_t tileX,
        const std::uint32_t tileY,
        const TileCell& cell,
        const TileDefinition& definition,
        Array<StaticCollider2D>& outStatics,
        SpatialHashGrid2D& outGrid) {
    if (cell.IsEmpty() || tileWorldSize <= 0.0F || !definition.ContributesCollision()) {
        return false;
    }

    const TileCollisionShape shape = definition.EffectiveCollisionShape();
    const std::uint8_t xf = cell.transformFlags;

    if (shape == TileCollisionShape::CustomConvex) {
        const std::size_t n = definition.customCollisionVertices.GetSize();
        if (n < 3) {
            return false;
        }
        Array<Vector2> worldVerts{};
        worldVerts.Reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            const Vector2 t = TransformNormalizedCellPoint(definition.customCollisionVertices[i], xf);
            Vector4 wh{};
            LocalTilePointToWorld(worldMatrix, tileWorldSize, tileX, tileY, t, wh);
            const Vector3 v = Hp3(wh);
            worldVerts.PushBack({v.x, v.y});
        }
        AppendConvexCollider(owner, collider, worldVerts, outStatics, outGrid);
        return true;
    }

    Vector2 corners[4]{};
    if (shape == TileCollisionShape::FullCell) {
        corners[0] = {0.0F, 0.0F};
        corners[1] = {1.0F, 0.0F};
        corners[2] = {1.0F, 1.0F};
        corners[3] = {0.0F, 1.0F};
    } else if (shape == TileCollisionShape::BottomHalf) {
        corners[0] = {0.0F, 0.0F};
        corners[1] = {1.0F, 0.0F};
        corners[2] = {1.0F, 0.5F};
        corners[3] = {0.0F, 0.5F};
    } else if (shape == TileCollisionShape::TopHalf) {
        corners[0] = {0.0F, 0.5F};
        corners[1] = {1.0F, 0.5F};
        corners[2] = {1.0F, 1.0F};
        corners[3] = {0.0F, 1.0F};
    } else {
        return false;
    }

    CollisionAabb2 aabb{};
    BuildWorldAabbFromNormalizedCorners(
            worldMatrix, tileWorldSize, tileX, tileY, xf, corners, 4, aabb);
    AppendBoxCollider(owner, collider, aabb, outStatics, outGrid);
    return true;
}

}  // namespace Spark
