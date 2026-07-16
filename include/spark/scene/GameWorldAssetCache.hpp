#pragma once

#include "spark/animation/Skeleton.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"
#include "spark/scene/Texture2D.hpp"

namespace Spark {

/** Result of loading a glTF asset (mesh + optional baseColor texture). */
struct GltfAsset {
    SharedPtr<Mesh> mesh;
    SharedPtr<Texture2D> baseColorTexture;
};

/** Skinned glTF: mesh + skeleton + animations + optional texture; walkClipIndex names a walk cycle when found. */
struct SkinnedGltfAsset {
    SharedPtr<SkinnedMesh> mesh;
    SharedPtr<Skeleton> skeleton;
    SharedPtr<Texture2D> baseColorTexture;
    std::uint32_t walkClipIndex = 0;
    Quaternion bindUpAlignment{Quaternion::Identity};
    float bindFacingYawOffset = 0.0F;
};

namespace Detail {

struct Utf8StringHasher {
    [[nodiscard]] std::size_t operator()(const Utf8String& s) const noexcept {
        const char* p = s.CStr();
        std::size_t h = 5381;
        while (*p != '\0') {
            h = ((h << 5) + h) + static_cast<unsigned char>(*p++);
        }
        return h;
    }
};

}  // namespace Detail

/**
 * Path-keyed mesh / glTF / texture caches (Single Responsibility: asset I/O, not entity graph).
 * Owned by <c>GameWorld</c> and shared with async loaders via register APIs.
 */
class GameWorldAssetCache {
public:
    [[nodiscard]] SharedPtr<Mesh> LoadMesh(const char* path);
    [[nodiscard]] GltfAsset LoadGltf(const char* path);
    void RegisterGltf(const GltfAsset& asset, const char* cacheKey);
    [[nodiscard]] SkinnedGltfAsset LoadSkinnedGltf(const char* path);
    void RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey);
    [[nodiscard]] SharedPtr<Texture2D> LoadTexture(const char* path);
    SharedPtr<Mesh> RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey = nullptr);
    SharedPtr<Texture2D> RegisterTexture(const SharedPtr<Texture2D>& texture, const char* cacheKey = nullptr);
    [[nodiscard]] SharedPtr<Mesh> TryGetMeshByKeyOrPath(const char* keyOrPath) const;
    [[nodiscard]] SharedPtr<Texture2D> TryGetTextureByKeyOrPath(const char* keyOrPath) const;
    [[nodiscard]] bool TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const;
    [[nodiscard]] bool TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax)
            const;

private:
    HashMap<Utf8String, SharedPtr<Mesh>, Detail::Utf8StringHasher> meshCache;
    HashMap<Utf8String, GltfAsset, Detail::Utf8StringHasher> gltfCache;
    HashMap<Utf8String, SkinnedGltfAsset, Detail::Utf8StringHasher> skinnedGltfCache;
    HashMap<Utf8String, SharedPtr<Texture2D>, Detail::Utf8StringHasher> textureCache;
};

}  // namespace Spark
