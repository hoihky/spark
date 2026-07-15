#pragma once

#include "spark/core/HashMap.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/Texture2D.hpp"

namespace Spark {

class Font;
class GameObject;
class IEngineContext;

/** Result of loading a glTF asset (mesh + optional baseColor texture). */
struct GltfAsset {
    SharedPtr<Mesh> mesh;
    SharedPtr<Texture2D> baseColorTexture;
};

class SkinnedMesh;
class Skeleton;

/** Skinned glTF: mesh + skeleton + animations + optional texture; walkClipIndex names a walk cycle when found. */
struct SkinnedGltfAsset {
    SharedPtr<SkinnedMesh> mesh;
    SharedPtr<Skeleton> skeleton;
    SharedPtr<Texture2D> baseColorTexture;
    std::uint32_t walkClipIndex = 0;
    /** Rotates baked mesh +Y onto world +Y so characters stand upright on the ground plane. */
    Quaternion bindUpAlignment{Quaternion::Identity};
    /**
     * Extra Y rotation (radians) so model forward matches CharacterCameraRig walk convention
     * (world -Z when visual yaw 0; same heading basis as atan2(move.x, -move.z)).
     */
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
 * Owns GameObjects (ECS entities), mesh asset cache, and the parent/child graph.
 */
class GameWorld {
public:
    GameWorld() = default;
    GameWorld(const GameWorld&) = delete;
    GameWorld& operator=(const GameWorld&) = delete;
    GameWorld(GameWorld&&) = delete;
    GameWorld& operator=(GameWorld&&) = delete;
    ~GameWorld();

    [[nodiscard]] GameObject* CreateGameObject();
    void DestroyGameObject(GameObject* object);

    /**
     * child and newParent must belong to this world. newParent may be nullptr (root).
     * Returns false if newParent is not in this world, or if linking would create a cycle.
     */
    bool SetParent(GameObject* child, GameObject* newParent);

    /** Per-frame component simulation (OnUpdate on each GameObject's components). */
    void UpdateGameObjects(const FrameTiming& timing, IEngineContext& context);

    /** Loads from path and caches by path; returns empty on failure. */
    [[nodiscard]] SharedPtr<Mesh> LoadMesh(const char* path);

    /** Loads .glb/.gltf (mesh + first PBR baseColor texture); cached by path. */
    [[nodiscard]] GltfAsset LoadGltf(const char* path);

    /** Inserts or replaces a rigid glTF cache entry (used by async asset loader). */
    void RegisterGltf(const GltfAsset& asset, const char* cacheKey);

    /** Loads first skinned mesh + skeleton + clips from glb/gltf; cached by path. */
    [[nodiscard]] SkinnedGltfAsset LoadSkinnedGltf(const char* path);

    /** Loads image from path and caches by path; returns empty on failure. */
    [[nodiscard]] SharedPtr<Texture2D> LoadTexture(const char* path);

    /** Uses an existing mesh instance; if cacheKey is non-null, stores it for later LoadMesh-style reuse. */
    SharedPtr<Mesh> RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey = nullptr);

    SharedPtr<Texture2D> RegisterTexture(const SharedPtr<Texture2D>& texture, const char* cacheKey = nullptr);

    /** Resolve mesh by cache key, or by path if that path was used with LoadGltf (glTF cache). */
    [[nodiscard]] SharedPtr<Mesh> TryGetMeshByKeyOrPath(const char* keyOrPath) const;

    /** Resolve texture by cache key, or glTF path (skinned cache preferred when both rigid and skinned exist). */
    [[nodiscard]] SharedPtr<Texture2D> TryGetTextureByKeyOrPath(const char* keyOrPath) const;

    /** After LoadSkinnedGltf(path), fetch the cached asset (mesh, skeleton, texture, walk clip). */
    [[nodiscard]] bool TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const;

    /** Inserts or replaces a skinned glTF cache entry (used by async asset loader). */
    void RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey);

    /** Rigid mesh from glTF/mesh cache, else axis-aligned bounds of skinned glTF vertices (bind pose). */
    [[nodiscard]] bool TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax)
            const;

    /** Font used for TextOverlayComponent / screen text when set on SceneRenderParams. */
    void SetUiFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiFont() const noexcept { return uiFont; }
    void SetUiBoldFont(SharedPtr<Font> font);
    [[nodiscard]] const SharedPtr<Font>& GetUiBoldFont() const noexcept { return uiBoldFont; }

    [[nodiscard]] std::size_t GetGameObjectCount() const noexcept { return objects.GetSize(); }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn) {
        for (std::size_t i = 0; i < objects.GetSize(); ++i) {
            fn(objects[i].Get());
        }
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn) const {
        for (std::size_t i = 0; i < objects.GetSize(); ++i) {
            fn(objects[i].Get());
        }
    }

private:
    friend class GameObject;

    void EraseGameObjectStorage(GameObject* pointer);
    static void CollectSubtreePostOrder(GameObject* root, Array<GameObject*>& out);

    Array<UniquePtr<GameObject>> objects;
    HashMap<Utf8String, SharedPtr<Mesh>, Detail::Utf8StringHasher> meshCache;
    HashMap<Utf8String, GltfAsset, Detail::Utf8StringHasher> gltfCache;
    HashMap<Utf8String, SkinnedGltfAsset, Detail::Utf8StringHasher> skinnedGltfCache;
    HashMap<Utf8String, SharedPtr<Texture2D>, Detail::Utf8StringHasher> textureCache;
    SharedPtr<Font> uiFont{};
    SharedPtr<Font> uiBoldFont{};
    std::uint64_t nextObjectId = 1;
};

}  // namespace Spark
