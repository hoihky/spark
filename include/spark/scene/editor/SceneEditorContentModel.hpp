#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/SceneEntityRole.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"

namespace Spark {

class GameWorld;
class MaterialComponent;

/** Hooks used when binding loaded scene entities into the editor model. */
struct SceneEditorContentBindingHooks {
    SharedPtr<Mesh>* unitCubeAsset = nullptr;
    void (*syncLightGizmoEmissive)(GameObject* lightObject) noexcept = nullptr;
};

/**
 * Editor-side model of placed content: tracks roots, meshes, lights, and spawn points.
 * Builds capture predicates for <c>SceneSerializer</c> (Model + Facade).
 */
class SceneEditorContentModel final {
public:
    void TrackRoot(GameObject* object) noexcept;
    void TrackPlaced(GameObject* object, Utf8String meshAssetRel = {}) noexcept;
    void TrackUserLight(GameObject* object) noexcept;
    void RemoveTracked(GameObject* object, GameWorld& world) noexcept;

    void ClearManualObjects(GameWorld& world) noexcept;
    void ClearLists() noexcept;

    [[nodiscard]] bool ShouldCapture(const GameObject* object) const noexcept;
    [[nodiscard]] SceneCaptureContext BuildCaptureContext() const noexcept;
    [[nodiscard]] Utf8String LookupMeshAssetRel(const GameObject& owner) const noexcept;
    [[nodiscard]] Utf8String LookupTextureAssetRel(const GameObject& owner) const noexcept;

    void IntegrateLoadedInstance(
            GameWorld& world,
            SceneInstanceId instanceId,
            const SceneDocument& document,
            const SceneEditorContentBindingHooks& hooks) noexcept;

    /** Re-registers tracking lists after hierarchy undo restores a subtree. */
    void IntegrateSubtree(GameObject& root) noexcept;

    /** Removes objects from tracking lists without destroying them. */
    void UntrackSubtree(const GameObject& root) noexcept;

    [[nodiscard]] GameObject* FindPlacedOwner(GameObject* object) const noexcept;
    [[nodiscard]] bool IsPlacedPrefabRoot(const GameObject* object) const noexcept;
    [[nodiscard]] bool IsInsidePlacedPrefab(const GameObject* object) const noexcept;

    [[nodiscard]] const Array<GameObject*>& GetRoots() const noexcept { return roots; }
    [[nodiscard]] Array<GameObject*>& GetRoots() noexcept { return roots; }
    [[nodiscard]] const Array<GameObject*>& GetPlacedObjects() const noexcept { return placed; }
    [[nodiscard]] const Array<Utf8String>& GetPlacedAssetPaths() const noexcept { return placedRel; }
    [[nodiscard]] const Array<GameObject*>& GetUserLights() const noexcept { return userLights; }
    [[nodiscard]] Array<GameObject*>& GetUserLights() noexcept { return userLights; }

private:
    void BindRole(
            GameObject* object,
            SceneEntityRoleInfo roleInfo,
            const SceneEditorContentBindingHooks& hooks) noexcept;

    Array<GameObject*> roots{};
    Array<GameObject*> placed{};
    Array<Utf8String> placedRel{};
    Array<GameObject*> userLights{};
};

}  // namespace Spark
