#include "spark/physics/MeshCollider3D.hpp"

#include "spark/ecs/components/physics/3d/MeshCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/Mesh.hpp"

#include <algorithm>

namespace Spark {

bool ContributesMeshCollider3DStatic(const GameObject& object) noexcept {
    const MeshCollider3DComponent* meshCol = object.GetComponent<MeshCollider3DComponent>();
    if (meshCol == nullptr) {
        return false;
    }
    if (object.GetComponent<MeshComponent>() == nullptr) {
        return false;
    }
    const Rigidbody3DComponent* rb = object.GetComponent<Rigidbody3DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType3D::Dynamic;
}

void ComputeMeshCollider3WorldAabb(
        GameObject& owner,
        const MeshCollider3DComponent& collider,
        CollisionAabb3& outWorld) noexcept {
    outWorld = CollisionAabb3{};
    const MeshComponent* meshComp = owner.GetComponent<MeshComponent>();
    if (meshComp == nullptr || !meshComp->GetMesh()) {
        return;
    }
    Vector3 localMin{};
    Vector3 localMax{};
    if (!meshComp->GetMesh()->TryComputeAxisAlignedBounds(localMin, localMax)) {
        return;
    }
    localMin += collider.GetLocalOffset();
    localMax += collider.GetLocalOffset();

    const Matrix4 worldM = owner.GetWorldMatrix();
    const Vector3 corners[8] = {
            worldM.TransformPoint({localMin.x, localMin.y, localMin.z}),
            worldM.TransformPoint({localMax.x, localMin.y, localMin.z}),
            worldM.TransformPoint({localMin.x, localMax.y, localMin.z}),
            worldM.TransformPoint({localMax.x, localMax.y, localMin.z}),
            worldM.TransformPoint({localMin.x, localMin.y, localMax.z}),
            worldM.TransformPoint({localMax.x, localMin.y, localMax.z}),
            worldM.TransformPoint({localMin.x, localMax.y, localMax.z}),
            worldM.TransformPoint({localMax.x, localMax.y, localMax.z}),
    };
    outWorld.minX = corners[0].x;
    outWorld.minY = corners[0].y;
    outWorld.minZ = corners[0].z;
    outWorld.maxX = corners[0].x;
    outWorld.maxY = corners[0].y;
    outWorld.maxZ = corners[0].z;
    for (int i = 1; i < 8; ++i) {
        outWorld.minX = std::min(outWorld.minX, corners[i].x);
        outWorld.minY = std::min(outWorld.minY, corners[i].y);
        outWorld.minZ = std::min(outWorld.minZ, corners[i].z);
        outWorld.maxX = std::max(outWorld.maxX, corners[i].x);
        outWorld.maxY = std::max(outWorld.maxY, corners[i].y);
        outWorld.maxZ = std::max(outWorld.maxZ, corners[i].z);
    }
}

}  // namespace Spark
