#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/assets/GameWorldAssetLoader.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/core/SceneSpawn.hpp"
#include "spark/scene/prefab/PrefabInstantiator.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

namespace Spark {

class GameWorld;

/** Options for SceneManager::LoadScene / BeginLoadSceneAsync. */
struct SceneLoadOptions {
    /** Base directory for relative mesh/texture paths in the file. */
    const char* assetsRoot = nullptr;
    /** When false, unloads all other scene instances before applying. */
    bool additive = true;
    /** Logical name stored in v4 header and instance record. */
    Utf8String sceneName;
};

/**
 * Runtime additive scene loading/unloading on a single GameWorld.
 * Call Pump each frame from OnUpdate while scenes are loading.
 */
class SceneManager {
public:
    explicit SceneManager(GameWorld& world);

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    [[nodiscard]] GameWorld& GetWorld() noexcept { return world; }
    [[nodiscard]] GameWorldAssetLoader& GetAssetLoader() noexcept { return world.GetAssetLoader(); }

    /**
     * Reads the scene file and instantiates entities. Asset decode runs on a worker thread;
     * this call pumps until all deferred components are restored or failure.
     */
    [[nodiscard]] SceneInstanceId LoadSceneFromFile(const char* path, const SceneLoadOptions& options = {});

    /**
     * Starts a load: entities + transforms are created immediately; mesh/texture components
     * finish when assets become ready. Poll with Pump / IsSceneReady.
     */
    [[nodiscard]] SceneInstanceId BeginLoadSceneAsync(const char* path, const SceneLoadOptions& options = {});

    /** Same as file overload but avoids re-reading when the document is already in memory. */
    [[nodiscard]] SceneInstanceId BeginLoadSceneAsync(
            const SceneDocument& document,
            const char* path,
            const SceneLoadOptions& options = {});

    /** Finishes in-flight asset loads and deferred component restores. */
    void Pump();

    [[nodiscard]] bool IsSceneReady(SceneInstanceId instanceId) const noexcept;
    [[nodiscard]] bool HasSceneFailed(SceneInstanceId instanceId) const noexcept;

    void UnloadScene(SceneInstanceId instanceId);
    void UnloadAllScenes();

    [[nodiscard]] std::size_t GetLoadedSceneCount() const noexcept { return instances.GetSize(); }

    /**
     * Loads a scene file and returns the pose of the named <c>SpawnPointComponent</c>.
     * When <c>spawnPointName</c> is null or empty, the first spawn point in the scene is used.
     */
    [[nodiscard]] SceneInstanceId LoadSceneAtSpawnPoint(
            const char* scenePath,
            const char* spawnPointName,
            SceneSpawnPose* outSpawnPose,
            const SceneLoadOptions& options = {});

    /** Convenience wrapper around <c>PrefabInstantiator::Instantiate</c>. */
    [[nodiscard]] PrefabInstantiateResult InstantiatePrefab(
            const char* prefabPath,
            const PrefabInstantiateOptions& options = {});

private:
    struct PendingComponentRestore {
        GameObject* object = nullptr;
        ComponentRecord record;
    };

    struct LoadedSceneInstance {
        SceneInstanceId id = kInvalidSceneInstanceId;
        Utf8String filePath;
        Utf8String name;
        bool ready = false;
        bool failed = false;
        Array<GameObject*> rootObjects;
        Array<PendingComponentRestore> pendingComponents;
        SceneLoadOptions options;
        Utf8String ownedAssetsRoot;
    };

    [[nodiscard]] SceneInstanceId AllocateInstanceId() noexcept { return nextInstanceId++; }

    [[nodiscard]] SceneInstanceId BeginLoadSceneInternal(const SceneDocument& document, const char* path, const SceneLoadOptions& options);

    void TagSubtreeSceneInstance(GameObject* root, SceneInstanceId id) noexcept;
    void ApplyDocumentInstance(
            const SceneDocument& document,
            LoadedSceneInstance& instance,
            const SceneLoadOptions& options);
    void RetryPendingComponents(LoadedSceneInstance& instance);
    [[nodiscard]] LoadedSceneInstance* FindInstance(SceneInstanceId id) noexcept;
    [[nodiscard]] const LoadedSceneInstance* FindInstance(SceneInstanceId id) const noexcept;
    void FinalizeInstanceReadyState(LoadedSceneInstance& instance);

    static void OnDeferredComponentStatic(GameObject* object, const ComponentRecord& record, void* userData);

    GameWorld& world;
    PrefabInstantiator prefabInstantiator;
    SceneInstanceId nextInstanceId = 1;
    Array<LoadedSceneInstance> instances;
};

}  // namespace Spark
