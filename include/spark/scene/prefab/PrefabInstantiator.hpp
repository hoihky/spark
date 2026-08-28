#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"

namespace Spark {

class GameObject;
class SceneManager;

/** Options when placing a <c>.sparkscene</c> prefab into a live world. */
struct PrefabInstantiateOptions {
    GameObject* parent = nullptr;
    Vector3 position{Vector3::Zero};
    Quaternion rotation = Quaternion::Identity;
    const char* assetsRoot = nullptr;
    bool additive = true;
    bool pumpUntilReady = true;
};

/** Result of a prefab instantiation (roots are top-level entities from the prefab file). */
struct PrefabInstantiateResult {
    SceneInstanceId instanceId = kInvalidSceneInstanceId;
    bool ready = false;
    Array<GameObject*> rootObjects{};
};

/**
 * Instantiates <c>.sparkscene</c> prefabs through a <c>SceneManager</c> (instance Strategy).
 * Prefer <c>SceneManager::InstantiatePrefab</c> at call sites; this type encapsulates pose logic.
 */
class PrefabInstantiator final {
public:
    explicit PrefabInstantiator(SceneManager& inManager) noexcept : manager(inManager) {}

    [[nodiscard]] PrefabInstantiateResult Instantiate(
            const char* prefabPath,
            const PrefabInstantiateOptions& options = {}) const;

private:
    SceneManager& manager;
};

}  // namespace Spark
