#include "spark/scene/SceneManager.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"

#include <cstring>

namespace Spark {

SceneManager::SceneManager(GameWorld& world) : world_(world) {
    assetLoader_.Start();
}

void SceneManager::OnDeferredComponentStatic(
        GameObject* object, const ComponentRecord& record, void* userData) {
    if (object == nullptr || userData == nullptr) {
        return;
    }
    auto* instance = static_cast<LoadedSceneInstance*>(userData);
    PendingComponentRestore pending{};
    pending.object = object;
    pending.record = record;
    instance->pendingComponents.PushBack(MoveTemp(pending));
}

void SceneManager::TagSubtreeSceneInstance(GameObject* root, const SceneInstanceId id) noexcept {
    if (root == nullptr) {
        return;
    }
    root->SetSceneInstanceId(id);
    const Array<GameObject*>& ch = root->GetChildren();
    for (std::size_t i = 0; i < ch.GetSize(); ++i) {
        TagSubtreeSceneInstance(ch[i], id);
    }
}

SceneManager::LoadedSceneInstance* SceneManager::FindInstance(const SceneInstanceId id) noexcept {
    for (std::size_t i = 0; i < instances_.GetSize(); ++i) {
        if (instances_[i].id == id) {
            return &instances_[i];
        }
    }
    return nullptr;
}

const SceneManager::LoadedSceneInstance* SceneManager::FindInstance(const SceneInstanceId id) const noexcept {
    for (std::size_t i = 0; i < instances_.GetSize(); ++i) {
        if (instances_[i].id == id) {
            return &instances_[i];
        }
    }
    return nullptr;
}

void SceneManager::RetryPendingComponents(LoadedSceneInstance& instance) {
    if (instance.pendingComponents.IsEmpty()) {
        return;
    }
    SceneApplyContext ctx{};
    ctx.assetsRoot = instance.options.assetsRoot;
    ctx.sceneInstanceId = instance.id;
    ctx.assetLoader = &assetLoader_;
    ctx.onDeferredComponent = nullptr;

    Array<PendingComponentRestore> stillPending;
    for (std::size_t i = 0; i < instance.pendingComponents.GetSize(); ++i) {
        PendingComponentRestore pending = instance.pendingComponents[i];
        if (pending.object == nullptr) {
            continue;
        }
        const IComponentSnapshotHandler* handler =
                ComponentSnapshotRegistry::Default().FindByTag(pending.record.kind.CStr());
        if (handler == nullptr) {
            continue;
        }
        if (handler->TryRestore(*pending.object, pending.record, world_, ctx)) {
            continue;
        }
        if (std::strcmp(pending.record.kind.CStr(), "skinned_mesh") != 0) {
            instance.failed = true;
        }
        stillPending.PushBack(MoveTemp(pending));
    }
    instance.pendingComponents = MoveTemp(stillPending);
}

void SceneManager::FinalizeInstanceReadyState(LoadedSceneInstance& instance) {
    if (instance.failed) {
        instance.ready = false;
        return;
    }
    if (instance.pendingComponents.IsEmpty()) {
        instance.ready = true;
    }
}

void SceneManager::ApplyDocumentInstance(
        const SceneDocument& document,
        LoadedSceneInstance& instance,
        const SceneLoadOptions& options) {
    SceneDeserializer deserializer;
    SceneApplyContext ctx{};
    ctx.assetsRoot = options.assetsRoot != nullptr && options.assetsRoot[0] != '\0' ? options.assetsRoot
                    : (!document.header.assetsRoot.IsEmpty() ? document.header.assetsRoot.CStr() : nullptr);
    ctx.sceneInstanceId = instance.id;
    ctx.assetLoader = &assetLoader_;
    ctx.onDeferredComponent = OnDeferredComponentStatic;
    ctx.deferredUserData = &instance;

    HashMap<std::uint64_t, GameObject*> idMap;
    if (!deserializer.Apply(document, world_, ctx, &idMap)) {
        instance.failed = true;
        return;
    }

    instance.rootObjects.Clear();
    for (std::size_t ei = 0; ei < document.entities.GetSize(); ++ei) {
        const EntityRecord& entity = document.entities[ei];
        if (entity.parentId >= 0) {
            continue;
        }
        if (GameObject* const* found = idMap.Find(entity.id); found != nullptr && *found != nullptr) {
            instance.rootObjects.PushBack(*found);
            TagSubtreeSceneInstance(*found, instance.id);
        }
    }
    RetryPendingComponents(instance);
    FinalizeInstanceReadyState(instance);
}

SceneInstanceId SceneManager::BeginLoadSceneInternal(
        const SceneDocument& document,
        const char* path,
        const SceneLoadOptions& options) {
    LoadedSceneInstance instance{};
    instance.id = AllocateInstanceId();
    instance.filePath = Utf8String(path != nullptr ? path : "");
    instance.name = !options.sceneName.IsEmpty() ? options.sceneName
                                                  : (!document.header.name.IsEmpty() ? document.header.name
                                                                                     : instance.filePath);
    instance.options = options;
    if (instance.options.assetsRoot == nullptr && !document.header.assetsRoot.IsEmpty()) {
        instance.options.assetsRoot = document.header.assetsRoot.CStr();
    }
    ApplyDocumentInstance(document, instance, options);
    instances_.PushBack(MoveTemp(instance));
    return instances_.GetLast().id;
}

SceneInstanceId SceneManager::LoadSceneFromFile(const char* path, const SceneLoadOptions& options) {
    if (path == nullptr) {
        return kInvalidSceneInstanceId;
    }
    SceneDocument document;
    SceneDeserializer deserializer;
    if (!deserializer.ReadFromFile(path, document)) {
        return kInvalidSceneInstanceId;
    }
    if (!options.additive) {
        UnloadAllScenes();
    }
    const SceneInstanceId id = BeginLoadSceneInternal(document, path, options);
    LoadedSceneInstance* instance = FindInstance(id);
    if (instance == nullptr) {
        return kInvalidSceneInstanceId;
    }
    constexpr int kMaxPumpIterations = 100000;
    for (int i = 0; i < kMaxPumpIterations && !instance->ready && !instance->failed; ++i) {
        Pump();
    }
    if (!instance->ready) {
        UnloadScene(id);
        return kInvalidSceneInstanceId;
    }
    return id;
}

SceneInstanceId SceneManager::BeginLoadSceneAsync(const char* path, const SceneLoadOptions& options) {
    if (path == nullptr) {
        return kInvalidSceneInstanceId;
    }
    SceneDocument document;
    SceneDeserializer deserializer;
    if (!deserializer.ReadFromFile(path, document)) {
        return kInvalidSceneInstanceId;
    }
    return BeginLoadSceneAsync(document, path, options);
}

SceneInstanceId SceneManager::BeginLoadSceneAsync(
        const SceneDocument& document,
        const char* path,
        const SceneLoadOptions& options) {
    if (path == nullptr) {
        return kInvalidSceneInstanceId;
    }
    if (!options.additive) {
        UnloadAllScenes();
    }
    return BeginLoadSceneInternal(document, path, options);
}

void SceneManager::Pump() {
    assetLoader_.Pump(world_);
    for (std::size_t i = 0; i < instances_.GetSize(); ++i) {
        LoadedSceneInstance& instance = instances_[i];
        if (instance.ready || instance.failed) {
            continue;
        }
        RetryPendingComponents(instance);
        FinalizeInstanceReadyState(instance);
    }
}

bool SceneManager::IsSceneReady(const SceneInstanceId instanceId) const noexcept {
    const LoadedSceneInstance* instance = FindInstance(instanceId);
    return instance != nullptr && instance->ready && !instance->failed;
}

bool SceneManager::HasSceneFailed(const SceneInstanceId instanceId) const noexcept {
    const LoadedSceneInstance* instance = FindInstance(instanceId);
    return instance != nullptr && instance->failed;
}

void SceneManager::UnloadScene(const SceneInstanceId instanceId) {
    if (instanceId == kInvalidSceneInstanceId) {
        return;
    }
    LoadedSceneInstance* instance = FindInstance(instanceId);
    if (instance == nullptr) {
        return;
    }
    Array<GameObject*> roots = instance->rootObjects;
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world_.DestroyGameObject(roots[i]);
        }
    }
    for (std::size_t i = 0; i < instances_.GetSize(); ++i) {
        if (instances_[i].id == instanceId) {
            instances_.RemoveAt(i);
            break;
        }
    }
}

void SceneManager::UnloadAllScenes() {
    while (!instances_.IsEmpty()) {
        const SceneInstanceId id = instances_.GetLast().id;
        UnloadScene(id);
    }
}

}  // namespace Spark
