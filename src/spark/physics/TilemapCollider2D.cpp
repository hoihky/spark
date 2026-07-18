#include "spark/physics/TilemapCollider2D.hpp"

#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Hp3(const Vector4& p) noexcept {
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

void ComputeTileWorldAabb(
        const Matrix4& worldMatrix,
        const float tileWorldSize,
        const std::uint32_t tileX,
        const std::uint32_t tileY,
        CollisionAabb2& outWorld) noexcept {
    const float x0 = static_cast<float>(tileX) * tileWorldSize;
    const float y0 = static_cast<float>(tileY) * tileWorldSize;
    const float x1 = x0 + tileWorldSize;
    const float y1 = y0 + tileWorldSize;

    const Vector4 corners[4] = {
            worldMatrix * Vector4(x0, y0, 0.0F, 1.0F),
            worldMatrix * Vector4(x1, y0, 0.0F, 1.0F),
            worldMatrix * Vector4(x1, y1, 0.0F, 1.0F),
            worldMatrix * Vector4(x0, y1, 0.0F, 1.0F),
    };

    const Vector3 v0 = Hp3(corners[0]);
    float minX = v0.x;
    float maxX = v0.x;
    float minY = v0.y;
    float maxY = v0.y;
    for (int i = 1; i < 4; ++i) {
        const Vector3 v = Hp3(corners[static_cast<std::size_t>(i)]);
        minX = std::min(minX, v.x);
        maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
    }

    outWorld.minX = minX;
    outWorld.maxX = maxX;
    outWorld.minY = minY;
    outWorld.maxY = maxY;
}

}  // namespace

bool ContributesTilemapCollider2DStatic(GameObject& object) noexcept {
    if (object.GetComponent<TilemapCollider2DComponent>() == nullptr) {
        return false;
    }
    if (object.GetComponent<TilemapComponent>() == nullptr) {
        return false;
    }
    const Rigidbody2DComponent* rb = object.GetComponent<Rigidbody2DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType2D::Dynamic;
}

void AppendTilemapCollider2DStatics(
        GameObject& owner,
        const TilemapCollider2DComponent& collider,
        const TilemapComponent& tilemap,
        Array<StaticCollider2D>& outStatics,
        SpatialHashGrid2D& outGrid) {
    const std::uint32_t mapWidth = tilemap.GetMapWidth();
    const std::uint32_t mapHeight = tilemap.GetMapHeight();
    const float tileWorldSize = tilemap.GetTileWorldSize();
    if (mapWidth == 0 || mapHeight == 0 || tileWorldSize <= 0.0F) {
        return;
    }

    const Matrix4 worldMatrix = owner.GetWorldMatrix();

    for (std::uint32_t y = 0; y < mapHeight; ++y) {
        for (std::uint32_t x = 0; x < mapWidth; ++x) {
            if (tilemap.GetTile(x, y) == TilemapComponent::kEmptyTile) {
                continue;
            }

            StaticCollider2D entry{};
            entry.shape = StaticCollider2DShape::Box;
            entry.categoryBits = collider.GetCategoryBits();
            entry.maskBits = collider.GetMaskBits();
            entry.owner = &owner;
            entry.isTrigger = collider.GetIsTrigger();
            ComputeTileWorldAabb(worldMatrix, tileWorldSize, x, y, entry.aabb);

            const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
            outStatics.PushBack(entry);
            outGrid.InsertIndexedAabb(idx, entry.aabb);
        }
    }
}

}  // namespace Spark
