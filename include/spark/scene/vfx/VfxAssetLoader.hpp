#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/assets/AssetLoadOutcome.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"

namespace Spark {

class GameWorldAssetCache;

/** Loads and saves <c>.sparkvfx</c> effect library files. */
class VfxAssetLoader {
public:
    [[nodiscard]] static AssetLoadOutcome<VfxAsset> TryLoadFromFile(
            const char* path,
            GameWorldAssetCache& cache);

    [[nodiscard]] static bool TrySaveToFile(const char* path, const VfxAsset& asset, const char* relativeToDir = nullptr);

    [[nodiscard]] static Utf8String ResolveReadablePath(const char* keyOrPath);
};

}  // namespace Spark
