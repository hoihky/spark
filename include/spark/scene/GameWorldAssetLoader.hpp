#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/scene/GameWorldAssetCache.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"
#include "spark/animation/Skeleton.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace Spark {

class GameWorld;

enum class AssetLoadJobKind : std::uint8_t {
    Gltf,
    SkinnedGltf,
    Texture,
    MeshObj,
};

enum class AssetLoadState : std::uint8_t {
    None,
    Queued,
    Loading,
    Ready,
    Failed,
};

/**
 * Background I/O + CPU decode for GameWorld asset caches.
 * Call Pump from the main thread each frame; GameWorld caches are updated only in Pump.
 */
class GameWorldAssetLoader {
public:
    GameWorldAssetLoader();
    ~GameWorldAssetLoader();

    GameWorldAssetLoader(const GameWorldAssetLoader&) = delete;
    GameWorldAssetLoader& operator=(const GameWorldAssetLoader&) = delete;

    void Start();
    void Shutdown();

    void RequestGltf(const char* path);
    void RequestSkinnedGltf(const char* path);
    void RequestTexture(const char* path);
    void RequestMeshObj(const char* path);

    [[nodiscard]] AssetLoadState GetState(const char* path, AssetLoadJobKind kind) const;
    [[nodiscard]] bool IsGltfReady(const char* path) const;
    [[nodiscard]] bool IsSkinnedGltfReady(const char* path) const;
    [[nodiscard]] bool IsTextureReady(const char* path) const;
    [[nodiscard]] bool IsMeshReady(const char* path) const;

    /** Commits completed worker jobs into <c>world</c> caches. */
    void Pump(GameWorld& world);

    [[nodiscard]] std::size_t GetPendingJobCount() const noexcept;

private:
    struct JobKey {
        Utf8String path;
        AssetLoadJobKind kind = AssetLoadJobKind::Gltf;

        [[nodiscard]] bool operator==(const JobKey& o) const noexcept {
            return kind == o.kind && path == o.path;
        }
    };

    struct JobKeyHasher {
        [[nodiscard]] std::size_t operator()(const JobKey& k) const noexcept {
            std::size_t h = static_cast<std::size_t>(k.kind);
            const char* p = k.path.CStr();
            while (p != nullptr && *p != '\0') {
                h = ((h << 5) + h) + static_cast<unsigned char>(*p++);
            }
            return h;
        }
    };

    struct CompletedGltf {
        Utf8String path;
        GltfAsset asset;
        bool ok = false;
    };

    struct CompletedSkinned {
        Utf8String path;
        SkinnedGltfAsset asset;
        bool ok = false;
    };

    struct CompletedTexture {
        Utf8String path;
        SharedPtr<Texture2D> texture;
        bool ok = false;
    };

    struct CompletedMesh {
        Utf8String path;
        SharedPtr<Mesh> mesh;
        bool ok = false;
    };

    struct PendingJob {
        JobKey key;
    };

    void WorkerLoop();
    void SetStateLocked(const JobKey& key, AssetLoadState state);
    [[nodiscard]] AssetLoadState GetStateLocked(const JobKey& key) const;

    mutable std::mutex mutex;
    std::condition_variable cv;
    std::thread worker;
    std::atomic<bool> stop{false};
    std::atomic<bool> started{false};

    Array<PendingJob> pendingJobs;
    Array<CompletedGltf> completedGltf;
    Array<CompletedSkinned> completedSkinned;
    Array<CompletedTexture> completedTextures;
    Array<CompletedMesh> completedMeshes;

    HashMap<JobKey, AssetLoadState, JobKeyHasher> states;
};

}  // namespace Spark
