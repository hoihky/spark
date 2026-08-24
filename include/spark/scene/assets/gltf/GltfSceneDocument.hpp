#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Transform.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

#include <cstdint>

namespace Spark {

class Skeleton;

/** One node in a parsed rigid glTF scene (flat array + parent index). */
struct GltfSceneNode {
    Utf8String name;
    Transform localTransform = Transform::Identity;
    std::int32_t parentIndex = -1;
    /** Index into <c>GltfSceneDocument::meshes</c>; <c>kInvalidMeshIndex</c> when empty. */
    static constexpr std::uint32_t kInvalidMeshIndex = UINT32_MAX;
    static constexpr std::uint32_t kInvalidSkinnedMeshIndex = UINT32_MAX;
    static constexpr std::uint32_t kInvalidSkeletonIndex = UINT32_MAX;
    std::uint32_t meshIndex = kInvalidMeshIndex;
    std::uint32_t skinnedMeshIndex = kInvalidSkinnedMeshIndex;
    std::uint32_t skeletonIndex = kInvalidSkeletonIndex;
    bool hasSkin = false;

    [[nodiscard]] bool HasMesh() const noexcept { return meshIndex != kInvalidMeshIndex; }
    [[nodiscard]] bool HasSkinnedMesh() const noexcept { return skinnedMeshIndex != kInvalidSkinnedMeshIndex; }
};

/**
 * Parsed rigid glTF scene: shared mesh table, materials, and node hierarchy.
 * Loaded once and instantiated into <c>GameObject</c> trees via <c>GltfSceneImporter</c>.
 */
struct GltfSceneDocument {
    Utf8String sourcePath;
    Array<SharedPtr<Mesh>> meshes;
    Array<SharedPtr<SkinnedMesh>> skinnedMeshes;
    Array<SharedPtr<Skeleton>> skeletons;
    Array<GltfMaterialDesc> materials;
    Array<GltfSceneNode> nodes;
    /** Indices into <c>nodes</c> for scene root nodes (parentIndex == -1). */
    Array<std::uint32_t> rootNodeIndices;

    [[nodiscard]] bool IsEmpty() const noexcept { return nodes.IsEmpty(); }
};

}  // namespace Spark
