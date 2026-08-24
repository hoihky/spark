#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/assets/gltf/GltfSceneDocument.hpp"

namespace Spark {

class GameObject;

/**
 * Instantiates a parsed <c>GltfSceneDocument</c> as a <c>GameObject</c> hierarchy
 * (per-node transforms; meshes in local space).
 */
class GltfSceneImporter {
public:
    /**
     * Attaches scene nodes under <c>owner</c>. When the document has a single root and
     * <c>owner</c> has no mesh yet, that root is applied directly to <c>owner</c>.
     */
    [[nodiscard]] static bool ImportInto(
            GameObject& owner,
            const GltfSceneDocument& document,
            SceneMeshSlot slot = SceneMeshSlot::Custom,
            const Vector3& albedo = Vector3::One,
            const char* gltfLibraryKey = nullptr);
};

}  // namespace Spark
