#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/GameWorldAssetCache.hpp"

namespace Spark {

class GameObject;

/**
 * Attaches loaded glTF assets to ECS objects (mesh + multi- or single-material setup).
 */
class GltfAssetBinder {
public:
    /** Applies glTF material slots to <c>owner</c> (multi- or single-material). */
    static void ApplyMaterials(GameObject& owner, const GltfAsset& asset);
    static void ApplyMaterials(GameObject& owner, const SkinnedGltfAsset& asset);

    static void BindRigidMesh(
          GameObject& owner,
          const GltfAsset& asset,
          SceneMeshSlot slot,
          const Vector3& albedo = Vector3::One);

  static void BindSkinnedMesh(
          GameObject& owner,
          const SkinnedGltfAsset& asset,
          const Vector3& albedo = Vector3::One);
};

}  // namespace Spark
