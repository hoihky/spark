#include "spark/scene/prefab/PrefabInstantiator.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Transform.hpp"
#include "spark/scene/core/SceneManager.hpp"

#include <cmath>

namespace Spark {

PrefabInstantiateResult PrefabInstantiator::Instantiate(
        const char* prefabPath,
        const PrefabInstantiateOptions& options) const {
    PrefabInstantiateResult result{};
    if (prefabPath == nullptr || prefabPath[0] == '\0') {
        return result;
    }

    SceneLoadOptions loadOpts{};
    loadOpts.additive = options.additive;
    loadOpts.assetsRoot = options.assetsRoot;
    loadOpts.sceneName = Utf8String(prefabPath);

    result.instanceId = manager.BeginLoadSceneAsync(prefabPath, loadOpts);
    if (result.instanceId == kInvalidSceneInstanceId) {
        return result;
    }

    if (options.pumpUntilReady) {
        constexpr int kMaxPumpIterations = 100000;
        for (int i = 0; i < kMaxPumpIterations; ++i) {
            manager.Pump();
            if (manager.IsSceneReady(result.instanceId)) {
                break;
            }
            if (manager.HasSceneFailed(result.instanceId)) {
                manager.UnloadScene(result.instanceId);
                result.instanceId = kInvalidSceneInstanceId;
                return result;
            }
        }
    }

    result.ready = manager.IsSceneReady(result.instanceId);
    if (!result.ready) {
        return result;
    }

    GameWorld& world = manager.GetWorld();
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr || object->GetSceneInstanceId() != result.instanceId) {
            return;
        }
        if (object->GetParent() != nullptr) {
            return;
        }
        result.rootObjects.PushBack(object);
    });

    const bool hasPoseOffset = options.position.LengthSquared() > 1.0e-8F || options.rotation.x != 0.0F ||
                               options.rotation.y != 0.0F || options.rotation.z != 0.0F ||
                               std::fabs(options.rotation.w - 1.0F) > 1.0e-6F;

    for (std::size_t ri = 0; ri < result.rootObjects.GetSize(); ++ri) {
        GameObject* root = result.rootObjects[ri];
        if (root == nullptr) {
            continue;
        }
        if (options.parent != nullptr) {
            root->SetParent(options.parent);
        }
        if (!hasPoseOffset) {
            continue;
        }
        TransformComponent* transform = root->GetComponent<TransformComponent>();
        if (transform == nullptr) {
            transform = root->AddComponent<TransformComponent>();
        }
        const Transform local = transform->GetLocalTransform();
        transform->SetTranslation(options.position + local.translation);
        transform->SetRotation(options.rotation * local.rotation);
    }

    return result;
}

}  // namespace Spark
