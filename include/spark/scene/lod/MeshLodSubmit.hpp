#pragma once

#include "spark/core/SharedPtr.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

#include <cstdint>

namespace Spark {

class MeshComponent;
class MeshLodComponent;
class SkinnedMeshComponent;

/** Effective drawable after LOD selection at scene submit. */
struct MeshLodResolvedDraw {
    std::uint32_t lodIndex = 0U;
    float cameraDistanceMeters = 0.0F;
    bool culled = false;
    SceneMeshSlot meshSlot = SceneMeshSlot::UnitCube;
    SharedPtr<Mesh> rigidMesh{};
    SharedPtr<SkinnedMesh> skinnedMesh{};
    std::uint32_t paletteUpdateStride = 1U;
};

/** World-space camera distance helpers (optional XZ-only for open-world props). */
[[nodiscard]] float MeshLodCameraDistanceMeters(
        const Vector3& cameraPositionWorld,
        const Matrix4& objectWorld,
        bool horizontalOnly) noexcept;

/** Rigid path: merges {@link MeshComponent} defaults with optional {@link MeshLodComponent}. */
[[nodiscard]] MeshLodResolvedDraw MeshLodResolveRigidDraw(
        const MeshComponent& mesh,
        MeshLodComponent* lod,
        const Vector3& cameraPositionWorld,
        const Matrix4& objectWorld) noexcept;

/** Skinned path: LOD mesh override + palette stride from {@link MeshLodComponent}. */
[[nodiscard]] MeshLodResolvedDraw MeshLodResolveSkinnedDraw(
        const SkinnedMeshComponent& skinned,
        MeshLodComponent* lod,
        const Vector3& cameraPositionWorld,
        const Matrix4& objectWorld) noexcept;

}  // namespace Spark