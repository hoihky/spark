#include "spark/physics/SpatialHashGrid3D.hpp"

#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/MeshCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/physics/MeshCollider3D.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorld.hpp"

#include <cmath>

namespace Spark {

namespace {

void ApplyMaterialToStaticRecord(const GameObject& object, StaticCollider3DSim& rec) {
    if (const PhysicsMaterial3DComponent* mat = object.GetComponent<PhysicsMaterial3DComponent>()) {
        rec.hasMaterial = true;
        rec.restitution = mat->GetRestitution();
        rec.staticFriction = mat->GetStaticFriction();
        rec.dynamicFriction = mat->GetDynamicFriction();
    }
}

void PushStaticBoxCollider(
        GameObject& object,
        const BoxCollider3DComponent& box,
        Array<StaticCollider3DSim>& outStatics,
        SpatialHashGrid3D& outGrid) {
    StaticCollider3DSim rec{};
    rec.shape = StaticCollider3DShape::Box;
    ComputeBoxCollider3WorldAabb(object, box, rec.aabb);
    ApplyMaterialToStaticRecord(object, rec);
    const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
    outStatics.PushBack(rec);
    outGrid.InsertIndexedAabb(idx, rec.aabb);
}

void PushStaticCapsuleCollider(
        GameObject& object,
        const CapsuleCollider3DComponent& capsule,
        Array<StaticCollider3DSim>& outStatics,
        SpatialHashGrid3D& outGrid) {
    StaticCollider3DSim rec{};
    rec.shape = StaticCollider3DShape::Capsule;
    ComputeCapsuleCollider3World(object, capsule, rec.capsule);
    ComputeCapsuleCollider3WorldAabb(object, capsule, rec.aabb);
    ApplyMaterialToStaticRecord(object, rec);
    const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
    outStatics.PushBack(rec);
    outGrid.InsertIndexedAabb(idx, rec.aabb);
}

}  // namespace

void SpatialHashGrid3D::Clear() noexcept {
    buckets.Clear();
    queryDedupe.Clear();
}

void SpatialHashGrid3D::SetCellSize(const float worldCellSize) noexcept {
    cellSize = (worldCellSize > 1.0e-4F) ? worldCellSize : 2.0F;
    invCellSize = 1.0F / cellSize;
}

void SpatialHashGrid3D::InsertIndexedAabb(const std::uint32_t payloadIndex, const CollisionAabb3& worldAabb) {
    const int ix0 = static_cast<int>(std::floor(worldAabb.minX * invCellSize));
    const int ix1 = static_cast<int>(std::floor(worldAabb.maxX * invCellSize));
    const int iy0 = static_cast<int>(std::floor(worldAabb.minY * invCellSize));
    const int iy1 = static_cast<int>(std::floor(worldAabb.maxY * invCellSize));
    const int iz0 = static_cast<int>(std::floor(worldAabb.minZ * invCellSize));
    const int iz1 = static_cast<int>(std::floor(worldAabb.maxZ * invCellSize));
    for (int iz = iz0; iz <= iz1; ++iz) {
        for (int iy = iy0; iy <= iy1; ++iy) {
            for (int ix = ix0; ix <= ix1; ++ix) {
                const CellKey key{ix, iy, iz};
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
}

void SpatialHashGrid3D::QueryUniquePayloadIndices(
        const CollisionAabb3& queryRegion,
        Array<std::uint32_t>& outUniqueIndices) const {
    outUniqueIndices.Clear();
    queryDedupe.Clear();

    const int ix0 = static_cast<int>(std::floor(queryRegion.minX * invCellSize));
    const int ix1 = static_cast<int>(std::floor(queryRegion.maxX * invCellSize));
    const int iy0 = static_cast<int>(std::floor(queryRegion.minY * invCellSize));
    const int iy1 = static_cast<int>(std::floor(queryRegion.maxY * invCellSize));
    const int iz0 = static_cast<int>(std::floor(queryRegion.minZ * invCellSize));
    const int iz1 = static_cast<int>(std::floor(queryRegion.maxZ * invCellSize));

    for (int iz = iz0; iz <= iz1; ++iz) {
        for (int iy = iy0; iy <= iy1; ++iy) {
            for (int ix = ix0; ix <= ix1; ++ix) {
                const CellKey key{ix, iy, iz};
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
}

void RebuildBroadPhaseFromStaticColliders3D(
        GameWorld& world,
        const float cellWorldSize,
        Array<StaticCollider3DSim>& outStatics,
        SpatialHashGrid3D& outGrid) {
    outStatics.Clear();
    outGrid.Clear();
    outGrid.SetCellSize(cellWorldSize);
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        if (!ContributesStaticCollider3D(*o)) {
            return;
        }
        if (const BoxCollider3DComponent* box = o->GetComponent<BoxCollider3DComponent>()) {
            PushStaticBoxCollider(*o, *box, outStatics, outGrid);
        }
        if (const CapsuleCollider3DComponent* capsule = o->GetComponent<CapsuleCollider3DComponent>()) {
            PushStaticCapsuleCollider(*o, *capsule, outStatics, outGrid);
        }
        if (ContributesMeshCollider3DStatic(*o)) {
            const MeshCollider3DComponent* meshCol = o->GetComponent<MeshCollider3DComponent>();
            if (meshCol != nullptr) {
                StaticCollider3DSim rec{};
                rec.shape = StaticCollider3DShape::Box;
                ComputeMeshCollider3WorldAabb(*o, *meshCol, rec.aabb);
                ApplyMaterialToStaticRecord(*o, rec);
                const std::uint32_t idx = static_cast<std::uint32_t>(outStatics.GetSize());
                outStatics.PushBack(rec);
                outGrid.InsertIndexedAabb(idx, rec.aabb);
            }
        }
    });
}

}  // namespace Spark
