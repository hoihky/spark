#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/assets/AssetLoadOutcome.hpp"
#include "spark/scene/material/MaterialAsset.hpp"
#include "spark/scene/serialization/MaterialSlotSnapshot.hpp"

namespace Spark {

class GameWorldAssetCache;
class Texture2D;

struct PendingMaterialTexture {
    Utf8String path;
    SharedPtr<Texture2D> texture;
};

/** Worker-thread decode result for <c>.sparkmat</c> (textures bound on the main thread). */
struct MaterialAssetFilePayload {
    MaterialAsset asset;
    MaterialSlotSnapshot::Data slot;
    Array<PendingMaterialTexture> pendingTextures;
};

/** Loads and saves <c>.sparkmat</c> material library files. */
class MaterialAssetLoader {
public:
    [[nodiscard]] static AssetLoadOutcome<MaterialAsset> TryLoadFromFile(
            const char* path,
            GameWorldAssetCache& cache);

    [[nodiscard]] static bool TryLoadFromFile(const char* path, MaterialAsset& out, GameWorldAssetCache& cache);

    /** Parses a <c>.sparkmat</c> file without touching the cache (safe for worker threads). */
    [[nodiscard]] static bool TryDecodeFromFile(const char* path, MaterialAssetFilePayload& out);

    /** Resolves slot texture keys/paths via cache and disk on the main thread. */
    static void ResolveMaterialTextures(
            GameWorldAssetCache& cache,
            const char* materialDiskPath,
            const MaterialSlotSnapshot::Data& slot,
            MaterialAsset& inOutAsset);

    /** Writes <c>sparkmat_v1</c> using paths relative to <c>relativeToDir</c> when possible. */
    [[nodiscard]] static bool TrySaveToFile(
            const char* path,
            const MaterialAsset& material,
            const char* relativeToDir = nullptr,
            const GameWorldAssetCache* textureKeys = nullptr);

    /** Resolves a logical asset key (e.g. <c>materials/hero.sparkmat</c>) to a readable file path. */
    [[nodiscard]] static Utf8String ResolveReadablePath(const char* keyOrPath);

    /** glTF-derived library key, e.g. <c>models/hero.glb#material/2</c>. */
    [[nodiscard]] static Utf8String MakeGltfMaterialLibraryKey(const char* gltfPath, std::size_t materialIndex);
};

}  // namespace Spark
