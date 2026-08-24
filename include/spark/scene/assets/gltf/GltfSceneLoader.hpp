#pragma once

#include "spark/scene/assets/AssetLoadOutcome.hpp"
#include "spark/scene/assets/gltf/GltfSceneDocument.hpp"

namespace Spark {

/** Parses rigid glTF files into a reusable <c>GltfSceneDocument</c> (meshes in local space). */
class GltfSceneLoader {
public:
    [[nodiscard]] static AssetLoadOutcome<GltfSceneDocument> TryLoadFromFile(const char* path) noexcept;
};

}  // namespace Spark
