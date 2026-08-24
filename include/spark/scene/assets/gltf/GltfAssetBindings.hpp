#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"

namespace Spark {

class GameObject;

/**
 * Attaches loaded glTF assets to ECS objects (mesh + multi- or single-material setup).
 */
class GltfAssetBinder {
public:
    /** Applies glTF material slots to <c>owner</c> (multi- or single-material). */
    static void ApplyMaterials(GameObject& owner, const GltfAsset& asset, const char* gltfLibraryKey = nullptr);
    static void ApplyMaterials(GameObject& owner, const SkinnedGltfAsset& asset, const char* gltfLibraryKey = nullptr);

    static void BindRigidMesh(
          GameObject& owner,
          const GltfAsset& asset,
          SceneMeshSlot slot,
          const Vector3& albedo = Vector3::One,
          const char* gltfLibraryKey = nullptr);

  static void BindSkinnedMesh(
          GameObject& owner,
          const SkinnedGltfAsset& asset,
          const Vector3& albedo = Vector3::One,
          const char* gltfLibraryKey = nullptr);

    /**
     * Probes <c>gltfPath</c>, loads rigid or skinned content, and attaches mesh + materials (+ skeleton for skinned).
     * Preferred entry point for displaying arbitrary glTF files.
     */
    [[nodiscard]] static bool BindFromPath(
            GameObject& owner,
            const char* gltfPath,
            SceneMeshSlot slot = SceneMeshSlot::Custom,
            const Vector3& albedo = Vector3::One);
};

}  // namespace Spark
