#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/SceneInstanceId.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class GameWorldAssetLoader;

/** Optional hooks while capturing entities from a live world. */
struct SceneCaptureContext {
    /** Returns relative asset path for MeshComponent (e.g. builtin:unit_cube, models/Foo.glb). */
    Utf8String (*resolveMeshAssetPath)(const GameObject& owner, void* userData) = nullptr;
    void* meshAssetUserData = nullptr;

    /** Returns relative skinned glTF path for SkinnedMeshComponent / AnimatorComponent. */
    Utf8String (*resolveSkinnedAssetPath)(const GameObject& owner, void* userData) = nullptr;
    void* skinnedAssetUserData = nullptr;

    /** Returns texture cache key or path for MaterialComponent base color. */
    Utf8String (*resolveTexturePath)(const GameObject& owner, void* userData) = nullptr;
    void* textureUserData = nullptr;
};

/** Optional hooks while instantiating entities into a live world. */
struct SceneApplyContext {
    const char* assetsRoot = nullptr;
    SceneInstanceId sceneInstanceId = kInvalidSceneInstanceId;
    /** When set, mesh/texture handlers may defer until assets are ready via Pump. */
    GameWorldAssetLoader* assetLoader = nullptr;
    void (*onEntityCreated)(GameObject* object, void* userData) = nullptr;
    void* entityUserData = nullptr;
    /** Called when a component restore is deferred pending async assets. */
    void (*onDeferredComponent)(GameObject* object, const ComponentRecord& record, void* userData) = nullptr;
    void* deferredUserData = nullptr;
};

/**
 * Writes and reads one component kind. Registered with ComponentSnapshotRegistry.
 * Implementations are stateless strategy objects (good OOP + open for extension).
 */
class IComponentSnapshotHandler {
public:
    virtual ~IComponentSnapshotHandler() = default;

    [[nodiscard]] virtual ComponentKind GetKind() const noexcept = 0;
    [[nodiscard]] virtual const char* GetKindTag() const noexcept = 0;

    [[nodiscard]] virtual bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            ComponentRecord& out) const = 0;

    [[nodiscard]] virtual bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& world,
            const SceneApplyContext& ctx) const = 0;
};

}  // namespace Spark
