#include "spark/physics/SpatialHashGrid2D.hpp"

#include "spark/ecs/components/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/CircleCollider2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorld.hpp"

#include <cmath>

namespace Spark {

void SpatialHashGrid2D::Clear() noexcept {
    buckets.Clear();
    queryDedupe.Clear();
}

void SpatialHashGrid2D::SetCellSize(float worldCellSize) noexcept {
    cellSize = (worldCellSize > 1.0e-4F) ? worldCellSize : 4.0F;
    invCellSize = 1.0F / cellSize;
}

void SpatialHashGrid2D::InsertIndexedAabb(std::uint32_t payloadIndex, const CollisionAabb2& worldAabb) {
    const float minX = worldAabb.minX;
    const float minY = worldAabb.minY;
    const float maxX = worldAabb.maxX;
    const float maxY = worldAabb.maxY;
    const int ix0 = static_cast<int>(std::floor(minX * invCellSize));
    const int ix1 = static_cast<int>(std::floor(maxX * invCellSize));
    const int iy0 = static_cast<int>(std::floor(minY * invCellSize));
    const int iy1 = static_cast<int>(std::floor(maxY * invCellSize));
    for (int iy = iy0; iy <= iy1; ++iy) {
        for (int ix = ix0; ix <= ix1; ++ix) {
            const CellKey key{ix, iy};
            if (Array<std::uint32_t>* arr = buckets.Find(key)) {
                arr->PushBack(payloadIndex);
            } else {
                Array<std::uint32_t> fresh;
                fresh.PushBack(payloadIndex);
                buckets.Add(key, MoveTemp(fresh));
            }
        }
    }
}

void SpatialHashGrid2D::QueryUniquePayloadIndices(
        const CollisionAabb2& queryRegion,
        Array<std::uint32_t>& outUniqueIndices) const {
    outUniqueIndices.Clear();
    queryDedupe.Clear();

    const int ix0 = static_cast<int>(std::floor(queryRegion.minX * invCellSize));
    const int ix1 = static_cast<int>(std::floor(queryRegion.maxX * invCellSize));
    const int iy0 = static_cast<int>(std::floor(queryRegion.minY * invCellSize));
    const int iy1 = static_cast<int>(std::floor(queryRegion.maxY * invCellSize));

    for (int iy = iy0; iy <= iy1; ++iy) {
        for (int ix = ix0; ix <= ix1; ++ix) {
            const CellKey key{ix, iy};
            const Array<std::uint32_t>* bucket = buckets.Find(key);
            if (bucket == nullptr) {
                continue;
            }
            for (std::size_t i = 0; i < bucket->GetSize(); ++i) {
                const std::uint32_t id = (*bucket)[i];
                if (queryDedupe.Find(id) != nullptr) {
                    continue;
                }
                queryDedupe.Add(id, 1);
                outUniqueIndices.PushBack(id);
            }
        }
    }
}

void RebuildBroadPhaseFromStaticColliders2D(
        GameWorld& world,
        const float cellWorldSize,
        Array<StaticCollider2D>& outStatics,
        SpatialHashGrid2D& outGrid) {
    outStatics.Clear();
    outGrid.Clear();
    outGrid.SetCellSize(cellWorldSize);
    world.ForEachGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        if (!ContributesStaticCollider2D(*o)) {
            return;
        }
        if (const BoxCollider2DComponent* col = o->GetComponent<BoxCollider2DComponent>()) {
            StaticCollider2D sc{};
            sc.shape = StaticCollider2DShape::Box;
            sc.categoryBits = col->GetCategoryBits();
            sc.maskBits = col->GetMaskBits();
            sc.owner = o;
            sc.isTrigger = col->GetIsTrigger();
            ComputeBoxCollider2WorldAabb(*o, *col, sc.aabb);
            const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
            outStatics.PushBack(sc);
            outGrid.InsertIndexedAabb(idx, sc.aabb);
        }
        if (const CircleCollider2DComponent* circ = o->GetComponent<CircleCollider2DComponent>()) {
            StaticCollider2D sc{};
            sc.shape = StaticCollider2DShape::Circle;
            sc.categoryBits = circ->GetCategoryBits();
            sc.maskBits = circ->GetMaskBits();
            sc.owner = o;
            sc.isTrigger = circ->GetIsTrigger();
            ComputeCircleCollider2World(*o, *circ, sc.circleCx, sc.circleCy, sc.circleR);
            const float rr = sc.circleR;
            sc.aabb.minX = sc.circleCx - rr;
            sc.aabb.maxX = sc.circleCx + rr;
            sc.aabb.minY = sc.circleCy - rr;
            sc.aabb.maxY = sc.circleCy + rr;
            const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
            outStatics.PushBack(sc);
            outGrid.InsertIndexedAabb(idx, sc.aabb);
        }
    });
}

}  // namespace Spark
