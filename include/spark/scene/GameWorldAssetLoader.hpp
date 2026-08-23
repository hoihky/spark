#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/scene/AssetLoadEvents.hpp"
#include "spark/scene/GameWorldAssetCache.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/scene/MaterialAssetLoader.hpp"
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
    void RequestMaterial(const char* path);

    /** Registers <c>callback</c> and starts the load when needed. Invoked on the main thread from <c>Pump</c>. */
    void OnGltfReady(const char* path, AssetLoadCallback callback);
    void OnSkinnedGltfReady(const char* path, AssetLoadCallback callback);
    void OnTextureReady(const char* path, AssetLoadCallback callback);
    void OnMaterialReady(const char* path, AssetLoadCallback callback);
    void OnAssetReady(const char* path, AssetLoadJobKind kind, AssetLoadCallback callback);

    void SetAssetLoadListener(IAssetLoadListener* listener) noexcept { globalListener = listener; }
    [[nodiscard]] IAssetLoadListener* GetAssetLoadListener() const noexcept { return globalListener; }

    [[nodiscard]] AssetLoadState GetState(const char* path, AssetLoadJobKind kind) const;
    [[nodiscard]] Utf8String GetLoadError(const char* path, AssetLoadJobKind kind) const;
    [[nodiscard]] bool IsGltfReady(const char* path) const;
    [[nodiscard]] bool IsSkinnedGltfReady(const char* path) const;
    [[nodiscard]] bool IsTextureReady(const char* path) const;
    [[nodiscard]] bool IsMeshReady(const char* path) const;
    [[nodiscard]] bool IsMaterialReady(const char* path) const;

    /** Commits completed worker jobs into <c>world</c> caches and dispatches load callbacks. */
    void InvalidateAssetLoadState(const char* path, AssetLoadJobKind kind);

    void Pump(GameWorld& world);

    /** Jobs waiting in the worker queue (not yet started). */
    [[nodiscard]] std::size_t GetPendingJobCount() const noexcept;
    /** Jobs that are queued or actively decoding on the worker thread. */
    [[nodiscard]] std::size_t GetOutstandingLoadCount() const noexcept;

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
        Utf8String errorMessage;
    };

    struct CompletedSkinned {
        Utf8String path;
        SkinnedGltfAsset asset;
        bool ok = false;
        Utf8String errorMessage;
    };

    struct CompletedTexture {
        Utf8String path;
        SharedPtr<Texture2D> texture;
        bool ok = false;
        Utf8String errorMessage;
    };

    struct CompletedMesh {
        Utf8String path;
        SharedPtr<Mesh> mesh;
        bool ok = false;
    };

    struct CompletedMaterial {
        Utf8String path;
        MaterialAssetFilePayload payload;
        bool ok = false;
        Utf8String errorMessage;
    };

    struct PendingJob {
        JobKey key;
    };

    void WorkerLoop();
    void RequestAsset(const JobKey& key, AssetLoadCallback callback);
    void DispatchCompletion(const JobKey& key, AssetLoadState state, const Utf8String& errorMessage);
    void SetStateLocked(const JobKey& key, AssetLoadState state);
    [[nodiscard]] AssetLoadState GetStateLocked(const JobKey& key) const;

    IAssetLoadListener* globalListener = nullptr;

    mutable std::mutex mutex;
    std::condition_variable cv;
    std::thread worker;
    std::atomic<bool> stop{false};
    std::atomic<bool> started{false};
    std::atomic<std::size_t> activeWorkerJobs{0};

    Array<PendingJob> pendingJobs;
    Array<CompletedGltf> completedGltf;
    Array<CompletedSkinned> completedSkinned;
    Array<CompletedTexture> completedTextures;
    Array<CompletedMesh> completedMeshes;
    Array<CompletedMaterial> completedMaterials;

    HashMap<JobKey, AssetLoadState, JobKeyHasher> states;
    HashMap<JobKey, Utf8String, JobKeyHasher> errors;
    HashMap<JobKey, Array<AssetLoadCallback>, JobKeyHasher> completionCallbacks;
};

}  // namespace Spark
