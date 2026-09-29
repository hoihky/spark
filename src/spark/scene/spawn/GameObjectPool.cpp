#include "spark/scene/spawn/GameObjectPool.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/SceneManager.hpp"
#include "spark/scene/prefab/PrefabInstantiator.hpp"

namespace Spark {

GameObjectPool::Bucket* GameObjectPool::FindOrCreateBucket(const char* prefabPath) {
    if (prefabPath == nullptr || prefabPath[0] == '\0') {
        return nullptr;
    }
    for (std::size_t i = 0; i < buckets.GetSize(); ++i) {
        if (buckets[i].prefabPath == Utf8String(prefabPath)) {
            return &buckets[i];
        }
    }
    Bucket bucket{};
    bucket.prefabPath = Utf8String(prefabPath);
    buckets.PushBack(bucket);
    return &buckets[buckets.GetSize() - 1U];
}

bool GameObjectPool::Prewarm(
        GameWorld& world,
        const char* prefabPath,
        const std::uint32_t count,
        GameObject* parent) {
    Bucket* bucket = FindOrCreateBucket(prefabPath);
    if (bucket == nullptr || count == 0U) {
        return false;
    }
    PrefabInstantiator instantiator(sceneManager);
    for (std::uint32_t i = 0; i < count; ++i) {
        PrefabInstantiateOptions options{};
        options.parent = parent;
        options.additive = true;
        const PrefabInstantiateResult result = instantiator.Instantiate(prefabPath, options);
        if (!result.ready || result.rootObjects.IsEmpty()) {
            continue;
        }
        for (std::size_t r = 0; r < result.rootObjects.GetSize(); ++r) {
            GameObject* root = result.rootObjects[r];
            if (root != nullptr) {
                root->SetActive(false);
                bucket->freeList.PushBack(root);
            }
        }
    }
    static_cast<void>(world);
    return true;
}

GameObject* GameObjectPool::Acquire(GameWorld& world, const char* prefabPath, GameObject* parent) {
    Bucket* bucket = FindOrCreateBucket(prefabPath);
    if (bucket == nullptr) {
        return nullptr;
    }
    while (!bucket->freeList.IsEmpty()) {
        GameObject* instance = bucket->freeList[bucket->freeList.GetSize() - 1U];
        bucket->freeList.PopBack();
        if (instance != nullptr) {
            instance->SetActive(true);
            return instance;
        }
    }
    PrefabInstantiator instantiator(sceneManager);
    PrefabInstantiateOptions options{};
    options.parent = parent;
    options.additive = true;
    const PrefabInstantiateResult result = instantiator.Instantiate(prefabPath, options);
    if (!result.ready || result.rootObjects.IsEmpty()) {
        return nullptr;
    }
    GameObject* root = result.rootObjects[0];
    static_cast<void>(world);
    return root;
}

void GameObjectPool::Release(const char* prefabPath, GameObject* instance) noexcept {
    if (instance == nullptr) {
        return;
    }
    instance->SetActive(false);
    Bucket* bucket = FindOrCreateBucket(prefabPath);
    if (bucket != nullptr) {
        bucket->freeList.PushBack(instance);
    }
}

}  // namespace Spark
