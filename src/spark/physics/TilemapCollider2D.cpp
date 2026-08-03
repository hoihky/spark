#include "spark/physics/TilemapCollider2D.hpp"

#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"
#include "spark/physics/TilemapCollisionBake.hpp"

namespace Spark {

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
        Array<Collider2D>& outColliders,
        SpatialHashGrid2D& outGrid) {
    const std::uint32_t mapWidth = tilemap.GetMapWidth();
    const std::uint32_t mapHeight = tilemap.GetMapHeight();
    const float tileWorldSize = tilemap.GetTileWorldSize();
    if (mapWidth == 0 || mapHeight == 0 || tileWorldSize <= 0.0F) {
        return;
    }

    const Matrix4 worldMatrix = owner.GetWorldMatrix();

    for (std::uint32_t layerIndex = 0; layerIndex < tilemap.GetLayerCount(); ++layerIndex) {
        const TilemapLayer& mapLayer = tilemap.GetLayer(layerIndex);
        if (!mapLayer.contributeCollision) {
            continue;
        }
        for (std::uint32_t y = 0; y < mapHeight; ++y) {
            for (std::uint32_t x = 0; x < mapWidth; ++x) {
                const TileCell cell = tilemap.GetTileCell(layerIndex, x, y);
                if (cell.IsEmpty()) {
                    continue;
                }
                const TileDefinition& definition = tilemap.GetTileDefinition(layerIndex, x, y);
                (void)AppendTilemapCellCollider2D(
                        owner,
                        collider,
                        worldMatrix,
                        tileWorldSize,
                        x,
                        y,
                        cell,
                        definition,
                        outColliders,
                        outGrid);
            }
        }
    }
}

}  // namespace Spark
