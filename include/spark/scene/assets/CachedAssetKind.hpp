#pragma once

#include "spark/scene/assets/AssetLoadEvents.hpp"

namespace Spark {

/** Discriminator for explicit cache retain / release (<c>GameWorldAssetCache</c>). */
enum class CachedAssetKind : std::uint8_t {
    Mesh,
    Texture,
    Material,
    Gltf,
    GltfScene,
    SkinnedGltf,
};

/** Maps a cached asset kind to its async loader job kind when applicable. */
[[nodiscard]] inline AssetLoadJobKind ToAssetLoadJobKind(const CachedAssetKind kind) noexcept {
    switch (kind) {
        case CachedAssetKind::Mesh:
            return AssetLoadJobKind::MeshObj;
        case CachedAssetKind::Texture:
            return AssetLoadJobKind::Texture;
        case CachedAssetKind::Material:
            return AssetLoadJobKind::Material;
        case CachedAssetKind::Gltf:
            return AssetLoadJobKind::Gltf;
        case CachedAssetKind::GltfScene:
            return AssetLoadJobKind::Gltf;
        case CachedAssetKind::SkinnedGltf:
            return AssetLoadJobKind::SkinnedGltf;
    }
    return AssetLoadJobKind::Gltf;
}

}  // namespace Spark
