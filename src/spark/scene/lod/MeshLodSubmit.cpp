#include "spark/scene/lod/MeshLodSubmit.hpp"

#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MeshLodComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/scene/lod/MeshLodSelector.hpp"

#include <cmath>

namespace Spark {

namespace {

Vector3 TranslationFromWorldMatrix(const Matrix4& world) noexcept {
    return {world.m[12], world.m[13], world.m[14]};
}

}  // namespace

float MeshLodCameraDistanceMeters(
        const Vector3& cameraPositionWorld,
        const Matrix4& objectWorld,
        const bool horizontalOnly) noexcept {
    const Vector3 p = TranslationFromWorldMatrix(objectWorld);
    if (horizontalOnly) {
        const float dx = p.x - cameraPositionWorld.x;
        const float dz = p.z - cameraPositionWorld.z;
        return std::sqrt(dx * dx + dz * dz);
    }
    return (p - cameraPositionWorld).Length();
}

MeshLodResolvedDraw MeshLodResolveRigidDraw(
        const MeshComponent& mesh,
        MeshLodComponent* lod,
        const Vector3& cameraPositionWorld,
        const Matrix4& objectWorld) noexcept {
    MeshLodResolvedDraw out{};
    out.meshSlot = mesh.GetSlot();
    out.rigidMesh = mesh.GetMesh();
    if (lod == nullptr || !lod->IsEnabled() || lod->GetLevels().IsEmpty()) {
        out.cameraDistanceMeters =
                MeshLodCameraDistanceMeters(cameraPositionWorld, objectWorld, lod != nullptr && lod->UsesHorizontalDistance());
        return out;
    }

    out.cameraDistanceMeters =
            MeshLodCameraDistanceMeters(cameraPositionWorld, objectWorld, lod->UsesHorizontalDistance());
    const std::uint32_t index = lod->SelectLodIndex(out.cameraDistanceMeters);
    out.lodIndex = index;
    const MeshLodLevel& level = lod->GetLevels()[index];
    out.culled = level.culled;
    out.paletteUpdateStride = std::max(1U, level.paletteUpdateStride);
    if (level.rigidMesh) {
        out.meshSlot = SceneMeshSlot::Custom;
        out.rigidMesh = level.rigidMesh;
    } else if (level.meshSlot != SceneMeshSlot::Custom) {
        out.meshSlot = level.meshSlot;
        out.rigidMesh = SharedPtr<Mesh>{};
    } else {
        out.meshSlot = mesh.GetSlot();
        out.rigidMesh = mesh.GetMesh();
    }
    return out;
}

MeshLodResolvedDraw MeshLodResolveSkinnedDraw(
        const SkinnedMeshComponent& skinned,
        MeshLodComponent* lod,
        const Vector3& cameraPositionWorld,
        const Matrix4& objectWorld) noexcept {
    MeshLodResolvedDraw out{};
    out.meshSlot = SceneMeshSlot::Custom;
    out.skinnedMesh = skinned.GetMesh();
    if (lod == nullptr || !lod->IsEnabled() || lod->GetLevels().IsEmpty()) {
        out.cameraDistanceMeters =
                MeshLodCameraDistanceMeters(cameraPositionWorld, objectWorld, lod != nullptr && lod->UsesHorizontalDistance());
        return out;
    }

    out.cameraDistanceMeters =
            MeshLodCameraDistanceMeters(cameraPositionWorld, objectWorld, lod->UsesHorizontalDistance());
    const std::uint32_t index = lod->SelectLodIndex(out.cameraDistanceMeters);
    out.lodIndex = index;
    const MeshLodLevel& level = lod->GetLevels()[index];
    out.culled = level.culled;
    out.paletteUpdateStride = std::max(1U, level.paletteUpdateStride);
    if (level.skinnedMesh) {
        out.skinnedMesh = level.skinnedMesh;
    }
    return out;
}

}  // namespace Spark