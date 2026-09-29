#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class SceneManager;

/**
 * Simple prefab pool for burst spawn (enemies, projectiles, VFX roots).
 * Inactive instances are deactivated; <c>Acquire</c> reactivates and teleports.
 */
class GameObjectPool final {
public:
    explicit GameObjectPool(SceneManager& sceneManagerIn) noexcept : sceneManager(sceneManagerIn) {}

    /** Pre-warms <c>count</c> instances of <c>prefabPath</c> under an optional parent. */
    bool Prewarm(GameWorld& world, const char* prefabPath, std::uint32_t count, GameObject* parent = nullptr);

    /** Returns an inactive instance or instantiates when the pool is empty. */
    [[nodiscard]] GameObject* Acquire(
            GameWorld& world,
            const char* prefabPath,
            GameObject* parent = nullptr);

    void Release(const char* prefabPath, GameObject* instance) noexcept;

private:
    struct Bucket {
        Utf8String prefabPath{};
        Array<GameObject*> freeList{};
    };

    [[nodiscard]] Bucket* FindOrCreateBucket(const char* prefabPath);

    SceneManager& sceneManager;
    Array<Bucket> buckets{};
};

}  // namespace Spark
