#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"

namespace Spark {

class GameObject;

/**
 * Presents full glTF PBR material tables from a <c>SkinnedGltfAsset</c> onto ECS components
 * (<c>MaterialComponent</c> or <c>MultiMaterialComponent</c>), matching rigid glTF import.
 */
class SkinnedGltfMaterialPresenter {
public:
    void PresentOn(GameObject& owner, const SkinnedGltfAsset& asset, const char* gltfLibraryKey = nullptr);

private:
    class SingleMaterialBinding;
    class MultiMaterialBinding;

    [[nodiscard]] bool ShouldUseMultiMaterial(const SkinnedGltfAsset& asset) const noexcept;
    [[nodiscard]] std::uint32_t ResolvePrimaryMaterialIndex(const SkinnedGltfAsset& asset) const noexcept;
    [[nodiscard]] const GltfMaterial& ResolveMaterial(
            const SkinnedGltfAsset& asset,
            std::uint32_t materialIndex) const noexcept;
    [[nodiscard]] Utf8String ResolveLibraryKey(const char* gltfLibraryKey) const noexcept;
};

}  // namespace Spark
