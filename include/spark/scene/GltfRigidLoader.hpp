#pragma once

#include "spark/core/Array.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GltfMaterial.hpp"
#include "spark/scene/Mesh.hpp"

namespace Spark {

/** CPU-side result of loading a rigid glTF scene into one indexed mesh. */
struct GltfRigidLoadResult {
    bool success = false;
    SharedPtr<Mesh> mesh;
    Array<GltfMaterial> materials;
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
