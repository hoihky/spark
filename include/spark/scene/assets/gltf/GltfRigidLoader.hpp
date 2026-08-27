#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/mesh/Mesh.hpp"

namespace Spark {

/** CPU-side result of loading a rigid glTF scene into one indexed mesh. */
struct GltfRigidLoadResult {
    bool success = false;
    Utf8String errorMessage;
    SharedPtr<Mesh> mesh;
    Array<GltfMaterial> materials;
    Array<Utf8String> materialVariantNames;
};

/**
 * Parses rigid glTF files: merges scene geometry into one mesh with per-primitive submeshes
 * and loads the full material table.
 */
class GltfRigidLoader {
public:
    [[nodiscard]] bool LoadFromFile(const char* path, GltfRigidLoadResult& out) noexcept;
};

}  // namespace Spark
