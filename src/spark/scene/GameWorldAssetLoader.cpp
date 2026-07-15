#include "spark/scene/GameWorldAssetLoader.hpp"

#include "spark/core/HashMap.hpp"
#include "spark/core/Utility.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/animation/Skeleton.hpp"

#include <cstring>

namespace Spark {

namespace {

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
    states.Clear();
}

void GameWorldAssetLoader::RequestGltf(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    if (!started.load()) {
        Start();
    }
    const JobKey key{Utf8String(path), AssetLoadJobKind::Gltf};
    std::lock_guard<std::mutex> lock(mutex);
    if (const AssetLoadState* st = states.Find(key); st != nullptr && *st != AssetLoadState::Failed) {
        return;
    }
    states.Add(key, AssetLoadState::Queued);
    PendingJob job{};
    job.key = key;
    pendingJobs.PushBack(job);
    cv.notify_one();
}

void GameWorldAssetLoader::RequestSkinnedGltf(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    if (!started.load()) {
        Start();
    }
    const JobKey key{Utf8String(path), AssetLoadJobKind::SkinnedGltf};
    std::lock_guard<std::mutex> lock(mutex);
    if (const AssetLoadState* st = states.Find(key); st != nullptr && *st != AssetLoadState::Failed) {
        return;
    }
    states.Add(key, AssetLoadState::Queued);
    PendingJob job{};
    job.key = key;
    pendingJobs.PushBack(job);
    cv.notify_one();
}

void GameWorldAssetLoader::RequestTexture(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return;
    }
    if (!started.load()) {
        Start();
    }
    const JobKey key{Utf8String(path), AssetLoadJobKind::Texture};
    std::lock_guard<std::mutex> lock(mutex);
    if (const AssetLoadState* st = states.Find(key); st != nullptr && *st != AssetLoadState::Failed) {
        return;
    }
    states.Add(key, AssetLoadState::Queued);
    PendingJob job{};
    job.key = key;
    pendingJobs.PushBack(job);
    cv.notify_one();
}

void GameWorldAssetLoader::RequestMeshObj(const char* path) {
    if (path == nullptr || path[0] == '\0' || IsGltfPath(path)) {
        return;
    }
    if (!started.load()) {
        Start();
    }
    const JobKey key{Utf8String(path), AssetLoadJobKind::MeshObj};
    std::lock_guard<std::mutex> lock(mutex);
    if (const AssetLoadState* st = states.Find(key); st != nullptr && *st != AssetLoadState::Failed) {
        return;
    }
    states.Add(key, AssetLoadState::Queued);
    PendingJob job{};
    job.key = key;
    pendingJobs.PushBack(job);
    cv.notify_one();
}

AssetLoadState GameWorldAssetLoader::GetState(const char* path, const AssetLoadJobKind kind) const {
    if (path == nullptr) {
        return AssetLoadState::None;
    }
    const JobKey key{Utf8String(path), kind};
    std::lock_guard<std::mutex> lock(mutex);
    return GetStateLocked(key);
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

void GameWorldAssetLoader::Pump(GameWorld& world) {
    Array<CompletedGltf> gltfBatch;
    Array<CompletedSkinned> skinnedBatch;
    Array<CompletedTexture> texBatch;
    Array<CompletedMesh> meshBatch;
    {
        std::lock_guard<std::mutex> lock(mutex);
        gltfBatch = MoveTemp(completedGltf);
        skinnedBatch = MoveTemp(completedSkinned);
        texBatch = MoveTemp(completedTextures);
        meshBatch = MoveTemp(completedMeshes);
        completedGltf.Clear();
        completedSkinned.Clear();
        completedTextures.Clear();
        completedMeshes.Clear();
    }

    for (std::size_t i = 0; i < gltfBatch.GetSize(); ++i) {
        const CompletedGltf& c = gltfBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::Gltf};
        if (c.ok && c.asset.mesh) {
            world.RegisterGltf(c.asset, c.path.CStr());
            std::lock_guard<std::mutex> lock(mutex);
            states.Add(key, AssetLoadState::Ready);
        } else {
            std::lock_guard<std::mutex> lock(mutex);
            states.Add(key, AssetLoadState::Failed);
        }
    }

    for (std::size_t i = 0; i < skinnedBatch.GetSize(); ++i) {
        const CompletedSkinned& c = skinnedBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::SkinnedGltf};
        if (c.ok && c.asset.mesh && c.asset.skeleton) {
            world.RegisterSkinnedGltf(c.asset, c.path.CStr());
            std::lock_guard<std::mutex> lock(mutex);
            states.Add(key, AssetLoadState::Ready);
        } else {
            std::lock_guard<std::mutex> lock(mutex);
            states.Add(key, AssetLoadState::Failed);
        }
    }

    for (std::size_t i = 0; i < texBatch.GetSize(); ++i) {
        const CompletedTexture& c = texBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::Texture};
        if (c.ok && c.texture) {
            world.RegisterTexture(c.texture, c.path.CStr());
            std::lock_guard<std::mutex> lock(mutex);
            states.Add(key, AssetLoadState::Ready);
        } else {
            std::lock_guard<std::mutex> lock(mutex);
            states.Add(key, AssetLoadState::Failed);
        }
    }

    for (std::size_t i = 0; i < meshBatch.GetSize(); ++i) {
        const CompletedMesh& c = meshBatch[i];
        const JobKey key{c.path, AssetLoadJobKind::MeshObj};
        if (c.ok && c.mesh) {
            world.RegisterMesh(c.mesh, c.path.CStr());
            std::lock_guard<std::mutex> lock(mutex);
            states.Add(key, AssetLoadState::Ready);
        } else {
            std::lock_guard<std::mutex> lock(mutex);
            states.Add(key, AssetLoadState::Failed);
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

        switch (job.key.kind) {
            case AssetLoadJobKind::Gltf: {
                CompletedGltf result{};
                result.path = job.key.path;
                auto mesh = MakeShared<Mesh>(job.key.path);
                SharedPtr<Texture2D> tex;
                result.ok = Mesh::TryLoadFromGltf(job.key.path.CStr(), *mesh, &tex);
                if (result.ok) {
                    result.asset.mesh = mesh;
                    result.asset.baseColorTexture = tex;
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
                SharedPtr<Texture2D> tex;
                std::uint32_t walkClip = 0;
                Quaternion bindUp{};
                float facingYaw = 0.0F;
                result.ok = TryLoadSkinnedCharacterFromGltf(
                        job.key.path.CStr(), mesh, skeleton, &tex, &walkClip, &bindUp, &facingYaw);
                if (result.ok) {
                    result.asset.mesh = MakeShared<SkinnedMesh>(MoveTemp(mesh));
                    result.asset.skeleton = MakeShared<Skeleton>(MoveTemp(skeleton));
                    result.asset.baseColorTexture = tex;
                    result.asset.walkClipIndex = walkClip;
                    result.asset.bindUpAlignment = bindUp;
                    result.asset.bindFacingYawOffset = facingYaw;
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
