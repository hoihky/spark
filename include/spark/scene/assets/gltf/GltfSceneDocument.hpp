#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Transform.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/mesh/Mesh.hpp"

#include <cstdint>

namespace Spark {

/** One node in a parsed rigid glTF scene (flat array + parent index). */
struct GltfSceneNode {
    Utf8String name;
    Transform localTransform = Transform::Identity;
    std::int32_t parentIndex = -1;
    /** Index into <c>GltfSceneDocument::meshes</c>; <c>kInvalidMeshIndex</c> when empty. */
    static constexpr std::uint32_t kInvalidMeshIndex = UINT32_MAX;
    std::uint32_t meshIndex = kInvalidMeshIndex;
    bool hasSkin = false;

    [[nodiscard]] bool HasMesh() const noexcept { return meshIndex != kInvalidMeshIndex; }
};

/**
 * Parsed rigid glTF scene: shared mesh table, materials, and node hierarchy.
 * Loaded once and instantiated into <c>GameObject</c> trees via <c>GltfSceneImporter</c>.
 */
struct GltfSceneDocument {
    Utf8String sourcePath;
    Array<SharedPtr<Mesh>> meshes;
    Array<GltfMaterialDesc> materials;
    Array<GltfSceneNode> nodes;
    /** Indices into <c>nodes</c> for scene root nodes (parentIndex == -1). */
    Array<std::uint32_t> rootNodeIndices;

    [[nodiscard]] bool IsEmpty() const noexcept { return nodes.IsEmpty(); }
};

}  // namespace Spark
