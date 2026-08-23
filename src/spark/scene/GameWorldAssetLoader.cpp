#include "spark/scene/GameWorldAssetLoader.hpp"

#include "spark/core/HashMap.hpp"
#include "spark/core/Utility.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/GltfMaterial.hpp"
#include "spark/scene/GltfRigidLoader.hpp"
#include "spark/scene/MaterialAssetLoader.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/animation/Skeleton.hpp"

#include <cstring>
#include <cstdio>
#include <sys/stat.h>

namespace Spark {

namespace {

bool IsRegularFile(const char* path) noexcept {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    struct stat st {};
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

[[nodiscard]] bool PathEndsWithInsensitive(const char* path, const char* suffix) {
    if (path == nullptr || suffix == nullptr) {
        return false;
    }
    const std::size_t lp = std::strlen(path);
    const std::size_t ls = std::strlen(suffix);
    if (lp < ls) {
        return false;
    }
    for (std::size_t i = 0; i < ls; ++i) {
        char a = path[lp - ls + i];
        char b = suffix[i];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IsGltfPath(const char* path) {
    return PathEndsWithInsensitive(path, ".glb") || PathEndsWithInsensitive(path, ".gltf");
}

}  // namespace

GameWorldAssetLoader::GameWorldAssetLoader() = default;

GameWorldAssetLoader::~GameWorldAssetLoader() {
    Shutdown();
}

void GameWorldAssetLoader::Start() {
    if (started.exchange(true)) {
        return;
    }
    stop.store(false);
    worker = std::thread([this]() { WorkerLoop(); });
}

void GameWorldAssetLoader::Shutdown() {
    if (!started.exchange(false)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex);
        stop.store(true);
    }
    cv.notify_all();
    if (worker.joinable()) {
        worker.join();
    }
    std::lock_guard<std::mutex> lock(mutex);
    pendingJobs.Clear();
    completedGltf.Clear();
    completedSkinned.Clear();
    completedTextures.Clear();
    completedMeshes.Clear();
    completedMaterials.Clear();
    states.Clear();
    errors.Clear();
    completionCallbacks.Clear();
    activeWorkerJobs.store(0, std::memory_order_release);
}

void GameWorldAssetLoader::RequestAsset(const JobKey& key, AssetLoadCallback callback) {
    if (key.path.IsEmpty()) {
        return;
    }
    if (!started.load()) {
        Start();
    }

    AssetLoadState existing = AssetLoadState::None;
    Utf8String existingError;
    bool dispatchReady = false;
    bool dispatchFailed = false;
    {
        std::lock_guard<std::mutex> lock(mutex);
        existing = GetStateLocked(key);
        if (existing == AssetLoadState::Ready) {
            dispatchReady = callback.IsBound();
        } else if (existing == AssetLoadState::Failed) {
            if (callback.IsBound()) {
                dispatchFailed = true;
                if (const Utf8String* err = errors.Find(key)) {
                    existingError = *err;
                }
            } else {
                existing = AssetLoadState::None;
            }
        }

        if (callback.IsBound()) {
            if (Array<AssetLoadCallback>* pending = completionCallbacks.Find(key)) {
                pending->PushBack(callback);
            } else {
                Array<AssetLoadCallback> batch{};
                batch.PushBack(callback);
                completionCallbacks.Add(key, MoveTemp(batch));
            }
        }

        if (existing == AssetLoadState::Ready || (existing == AssetLoadState::Failed && callback.IsBound())) {
            // Immediate dispatch after releasing the lock.
        } else if (existing == AssetLoadState::Queued || existing == AssetLoadState::Loading) {
            return;
        } else {
            states.Add(key, AssetLoadState::Queued);
            PendingJob job{};
            job.key = key;
            pendingJobs.PushBack(job);
            cv.notify_one();
        }
    }

    if (dispatchReady) {
        DispatchCompletion(key, AssetLoadState::Ready, {});
    } else if (dispatchFailed) {
        DispatchCompletion(key, AssetLoadState::Failed, existingError);
    }
}

void GameWorldAssetLoader::RequestGltf(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::Gltf}, {});
}

void GameWorldAssetLoader::RequestSkinnedGltf(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::SkinnedGltf}, {});
}

void GameWorldAssetLoader::RequestTexture(const char* path) {
    if (path == nullptr || path[0] == '\0' || !IsRegularFile(path)) {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::Texture}, {});
}

void GameWorldAssetLoader::InvalidateAssetLoadState(const char* path, const AssetLoadJobKind kind) {
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    const JobKey key{Utf8String(path), kind};
    std::lock_guard<std::mutex> lock(mutex);
    states.Remove(key);
    errors.Remove(key);
}

void GameWorldAssetLoader::RequestMaterial(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::Material}, {});
}

void GameWorldAssetLoader::OnMaterialReady(const char* path, AssetLoadCallback callback) {
    if (path == nullptr || path[0] == '\0' || !callback.IsBound()) {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::Material}, callback);
}

void GameWorldAssetLoader::RequestMeshObj(const char* path) {
    if (path == nullptr || path[0] == '\0' || IsGltfPath(path)) {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::MeshObj}, {});
}

void GameWorldAssetLoader::OnGltfReady(const char* path, AssetLoadCallback callback) {
    if (path == nullptr || path[0] == '\0' || !callback.IsBound()) {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::Gltf}, callback);
}

void GameWorldAssetLoader::OnSkinnedGltfReady(const char* path, AssetLoadCallback callback) {
    if (path == nullptr || path[0] == '\0' || !callback.IsBound()) {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::SkinnedGltf}, callback);
}

void GameWorldAssetLoader::OnTextureReady(const char* path, AssetLoadCallback callback) {
    if (path == nullptr || path[0] == '\0' || !callback.IsBound()) {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), AssetLoadJobKind::Texture}, callback);
}

void GameWorldAssetLoader::OnAssetReady(const char* path, const AssetLoadJobKind kind, AssetLoadCallback callback) {
    if (path == nullptr || path[0] == '\0' || !callback.IsBound()) {
        return;
    }
    RequestAsset(JobKey{Utf8String(path), kind}, callback);
}

AssetLoadState GameWorldAssetLoader::GetState(const char* path, const AssetLoadJobKind kind) const {
    if (path == nullptr) {
        return AssetLoadState::None;
    }
    const JobKey key{Utf8String(path), kind};
    std::lock_guard<std::mutex> lock(mutex);
    return GetStateLocked(key);
}

Utf8String GameWorldAssetLoader::GetLoadError(const char* path, const AssetLoadJobKind kind) const {
    if (path == nullptr) {
        return {};
    }
    const JobKey key{Utf8String(path), kind};
    std::lock_guard<std::mutex> lock(mutex);
    if (const Utf8String* err = errors.Find(key)) {
        return *err;
    }
    return {};
}

bool GameWorldAssetLoader::IsGltfReady(const char* path) const {
    return GetState(path, AssetLoadJobKind::Gltf) == AssetLoadState::Ready;
}

bool GameWorldAssetLoader::IsSkinnedGltfReady(const char* path) const {
    return GetState(path, AssetLoadJobKind::SkinnedGltf) == AssetLoadState::Ready;
}

bool GameWorldAssetLoader::IsTextureReady(const char* path) const {
    return GetState(path, AssetLoadJobKind::Texture) == AssetLoadState::Ready;
}

bool GameWorldAssetLoader::IsMeshReady(const char* path) const {
    return GetState(path, AssetLoadJobKind::MeshObj) == AssetLoadState::Ready;
}

std::size_t GameWorldAssetLoader::GetPendingJobCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    return pendingJobs.GetSize();
}

std::size_t GameWorldAssetLoader::GetOutstandingLoadCount() const noexcept {
    return GetPendingJobCount() + activeWorkerJobs.load(std::memory_order_acquire);
}

void GameWorldAssetLoader::DispatchCompletion(
        const JobKey& key,
        const AssetLoadState state,
        const Utf8String& errorMessage) {
    Array<AssetLoadCallback> callbacks;
    IAssetLoadListener* listener = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (Array<AssetLoadCallback>* pending = completionCallbacks.Find(key)) {
            callbacks = MoveTemp(*pending);
            pending->Clear();
        }
        listener = globalListener;
    }

    AssetLoadEvent event{};
    event.path = key.path;
    event.kind = key.kind;
    event.state = state;
    event.errorMessage = errorMessage;

    for (std::size_t i = 0; i < callbacks.GetSize(); ++i) {
        callbacks[i].Invoke(event);
    }
    if (listener != nullptr) {
        listener->OnAssetLoadCompleted(event);
    }
}

bool GameWorldAssetLoader::IsMaterialReady(const char* path) const {
    return GetState(path, AssetLoadJobKind::Material) == AssetLoadState::Ready;
}

void GameWorldAssetLoader::Pump(GameWorld& world) {
    Array<CompletedGltf> gltfBatch;
    Array<CompletedSkinned> skinnedBatch;
    Array<CompletedTexture> texBatch;
    Array<CompletedMesh> meshBatch;
    Array<CompletedMaterial> materialBatch;
    {
        std::lock_guard<std::mutex> lock(mutex);
        gltfBatch = MoveTemp(completedGltf);
        skinnedBatch = MoveTemp(completedSkinned);
        texBatch = MoveTemp(completedTextures);
        meshBatch = MoveTemp(completedMeshes);
        materialBatch = MoveTemp(completedMaterials);
        completedGltf.Clear();
        completedSkinned.Clear();
        completedTextures.Clear();
        completedMeshes.Clear();
        completedMaterials.Clear();
    }

    for (std::size_t i = 0; i < gltfBatch.GetSize(); ++i) {
        const CompletedGltf& c = gltfBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::Gltf};
        activeWorkerJobs.fetch_sub(1, std::memory_order_acq_rel);
        if (c.ok && c.asset.mesh) {
            world.RegisterGltf(c.asset, c.path.CStr());
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Ready);
            }
            DispatchCompletion(key, AssetLoadState::Ready, {});
        } else {
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Failed);
                if (!c.errorMessage.IsEmpty()) {
                    errors.Add(key, c.errorMessage);
                }
            }
            if (!c.errorMessage.IsEmpty()) {
                std::fprintf(stderr, "Spark: async glTF load failed: %s\n", c.errorMessage.CStr());
            }
            DispatchCompletion(key, AssetLoadState::Failed, c.errorMessage);
        }
    }

    for (std::size_t i = 0; i < skinnedBatch.GetSize(); ++i) {
        const CompletedSkinned& c = skinnedBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::SkinnedGltf};
        activeWorkerJobs.fetch_sub(1, std::memory_order_acq_rel);
        if (c.ok && c.asset.mesh && c.asset.skeleton) {
            world.RegisterSkinnedGltf(c.asset, c.path.CStr());
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Ready);
            }
            DispatchCompletion(key, AssetLoadState::Ready, {});
        } else {
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Failed);
                if (!c.errorMessage.IsEmpty()) {
                    errors.Add(key, c.errorMessage);
                }
            }
            if (!c.errorMessage.IsEmpty()) {
                std::fprintf(stderr, "Spark: async skinned glTF load failed: %s\n", c.errorMessage.CStr());
            }
            DispatchCompletion(key, AssetLoadState::Failed, c.errorMessage);
        }
    }

    for (std::size_t i = 0; i < texBatch.GetSize(); ++i) {
        const CompletedTexture& c = texBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::Texture};
        activeWorkerJobs.fetch_sub(1, std::memory_order_acq_rel);
        if (c.ok && c.texture) {
            world.RegisterTexture(c.texture, c.path.CStr());
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Ready);
            }
            DispatchCompletion(key, AssetLoadState::Ready, {});
        } else {
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Failed);
                if (!c.errorMessage.IsEmpty()) {
                    errors.Add(key, c.errorMessage);
                }
            }
            if (!c.errorMessage.IsEmpty()) {
                std::fprintf(stderr, "Spark: async texture load failed: %s\n", c.errorMessage.CStr());
            }
            DispatchCompletion(key, AssetLoadState::Failed, c.errorMessage);
        }
    }

    for (std::size_t i = 0; i < meshBatch.GetSize(); ++i) {
        const CompletedMesh& c = meshBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::MeshObj};
        activeWorkerJobs.fetch_sub(1, std::memory_order_acq_rel);
        if (c.ok && c.mesh) {
            world.RegisterMesh(c.mesh, c.path.CStr());
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Ready);
            }
            DispatchCompletion(key, AssetLoadState::Ready, {});
        } else {
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Failed);
            }
            DispatchCompletion(key, AssetLoadState::Failed, {});
        }
    }

    for (std::size_t i = 0; i < materialBatch.GetSize(); ++i) {
        const CompletedMaterial& c = materialBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::Material};
        activeWorkerJobs.fetch_sub(1, std::memory_order_acq_rel);
        if (c.ok) {
            MaterialAsset asset = c.payload.asset;
            const Utf8String resolvedPath = MaterialAssetLoader::ResolveReadablePath(c.path.CStr());
            MaterialAssetLoader::ResolveMaterialTextures(
                    world.GetAssetCache(), resolvedPath.CStr(), c.payload.slot, asset);
            world.RegisterMaterial(asset, c.path.CStr());
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Ready);
            }
            DispatchCompletion(key, AssetLoadState::Ready, {});
        } else {
            {
                std::lock_guard<std::mutex> lock(mutex);
                states.Add(key, AssetLoadState::Failed);
                if (!c.errorMessage.IsEmpty()) {
                    errors.Add(key, c.errorMessage);
                }
            }
            if (!c.errorMessage.IsEmpty()) {
                std::fprintf(stderr, "Spark: async material load failed: %s\n", c.errorMessage.CStr());
            }
            DispatchCompletion(key, AssetLoadState::Failed, c.errorMessage);
        }
    }
}

void GameWorldAssetLoader::WorkerLoop() {
    for (;;) {
        PendingJob job{};
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]() { return stop.load() || !pendingJobs.IsEmpty(); });
            if (stop.load() && pendingJobs.IsEmpty()) {
                return;
            }
            job = pendingJobs[0];
            pendingJobs.RemoveAt(0);
            SetStateLocked(job.key, AssetLoadState::Loading);
        }
        activeWorkerJobs.fetch_add(1, std::memory_order_acq_rel);

        switch (job.key.kind) {
            case AssetLoadJobKind::Gltf: {
                CompletedGltf result{};
                result.path = job.key.path;
                GltfRigidLoadResult loaded{};
                result.ok = GltfRigidLoader{}.LoadFromFile(job.key.path.CStr(), loaded);
                if (result.ok && loaded.mesh) {
                    result.asset.mesh = loaded.mesh;
                    result.asset.materials = loaded.materials;
                    if (!loaded.materials.IsEmpty()) {
                        result.asset.material = loaded.materials[0];
                        result.asset.baseColorTexture = loaded.materials[0].baseColor;
                    }
                } else {
                    result.ok = false;
                    result.errorMessage = loaded.errorMessage;
                }
                std::lock_guard<std::mutex> lock(mutex);
                completedGltf.PushBack(MoveTemp(result));
                break;
            }
            case AssetLoadJobKind::SkinnedGltf: {
                CompletedSkinned result{};
                result.path = job.key.path;
                SkinnedMesh mesh;
                Skeleton skeleton;
                GltfMaterialDesc material{};
                Array<GltfMaterialDesc> materials;
                std::uint32_t walkClip = 0;
                Quaternion bindUp{};
                float facingYaw = 0.0F;
                Utf8String loadError;
                result.ok = TryLoadSkinnedCharacterFromGltf(
                        job.key.path.CStr(),
                        mesh,
                        skeleton,
                        nullptr,
                        &material,
                        &walkClip,
                        &bindUp,
                        &facingYaw,
                        &materials,
                        &loadError);
                if (result.ok) {
                    result.asset.mesh = MakeShared<SkinnedMesh>(MoveTemp(mesh));
                    result.asset.skeleton = MakeShared<Skeleton>(MoveTemp(skeleton));
                    result.asset.materials = materials;
                    result.asset.material = material;
                    result.asset.baseColorTexture = material.baseColor;
                    result.asset.walkClipIndex = walkClip;
                    result.asset.bindUpAlignment = bindUp;
                    result.asset.bindFacingYawOffset = facingYaw;
                } else {
                    result.errorMessage = loadError;
                }
                std::lock_guard<std::mutex> lock(mutex);
                completedSkinned.PushBack(MoveTemp(result));
                break;
            }
            case AssetLoadJobKind::Texture: {
                CompletedTexture result{};
                result.path = job.key.path;
                auto tex = MakeShared<Texture2D>();
                result.ok = Texture2D::TryLoadFromFile(job.key.path.CStr(), *tex);
                if (result.ok) {
                    result.texture = tex;
                } else {
                    result.errorMessage = Utf8String("Failed to decode texture: ");
                    result.errorMessage.AppendUtf8(job.key.path.CStr());
                }
                std::lock_guard<std::mutex> lock(mutex);
                completedTextures.PushBack(MoveTemp(result));
                break;
            }
            case AssetLoadJobKind::MeshObj: {
                CompletedMesh result{};
                result.path = job.key.path;
                auto mesh = MakeShared<Mesh>(job.key.path);
                result.ok = Mesh::TryLoadFromObj(job.key.path.CStr(), *mesh);
                if (result.ok) {
                    result.mesh = mesh;
                }
                std::lock_guard<std::mutex> lock(mutex);
                completedMeshes.PushBack(MoveTemp(result));
                break;
            }
            case AssetLoadJobKind::Material: {
                CompletedMaterial result{};
                result.path = job.key.path;
                result.ok = MaterialAssetLoader::TryDecodeFromFile(job.key.path.CStr(), result.payload);
                if (!result.ok) {
                    result.errorMessage = Utf8String("Failed to decode material asset: ");
                    result.errorMessage.AppendUtf8(job.key.path.CStr());
                }
                std::lock_guard<std::mutex> lock(mutex);
                completedMaterials.PushBack(MoveTemp(result));
                break;
            }
        }
    }
}

void GameWorldAssetLoader::SetStateLocked(const JobKey& key, const AssetLoadState state) {
    states.Add(key, state);
}

AssetLoadState GameWorldAssetLoader::GetStateLocked(const JobKey& key) const {
    if (const AssetLoadState* st = states.Find(key)) {
        return *st;
    }
    return AssetLoadState::None;
}

}  // namespace Spark
