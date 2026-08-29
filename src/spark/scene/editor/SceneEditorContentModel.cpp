#include "spark/scene/editor/SceneEditorContentModel.hpp"

#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cstring>

namespace Spark {

namespace {

struct CaptureUserData {
    const SceneEditorContentModel* model = nullptr;
};

Utf8String ResolveMeshAssetPath(const GameObject& owner, void* userData) {
    const auto* data = static_cast<const CaptureUserData*>(userData);
    if (data == nullptr || data->model == nullptr) {
        return {};
    }
    return data->model->LookupMeshAssetRel(owner);
}

Utf8String ResolveTexturePath(const GameObject& owner, void* userData) {
    const auto* data = static_cast<const CaptureUserData*>(userData);
    if (data == nullptr || data->model == nullptr) {
        return {};
    }
    return data->model->LookupTextureAssetRel(owner);
}

}  // namespace

void SceneEditorContentModel::TrackRoot(GameObject* object) noexcept {
    if (object == nullptr) {
        return;
    }
    roots.PushBack(object);
}

void SceneEditorContentModel::TrackPlaced(GameObject* object, Utf8String meshAssetRel) noexcept {
    if (object == nullptr) {
        return;
    }
    placed.PushBack(object);
    placedRel.PushBack(MoveTemp(meshAssetRel));
}

void SceneEditorContentModel::TrackUserLight(GameObject* object) noexcept {
    if (object == nullptr) {
        return;
    }
    userLights.PushBack(object);
}

void SceneEditorContentModel::RemoveTracked(GameObject* object, GameWorld& world) noexcept {
    if (object == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < placed.GetSize(); ++i) {
        if (placed[i] == object) {
            world.DestroyGameObject(object);
            placed.RemoveAt(i);
            if (i < placedRel.GetSize()) {
                placedRel.RemoveAt(i);
            }
            return;
        }
    }
    for (std::size_t i = 0; i < userLights.GetSize(); ++i) {
        if (userLights[i] == object) {
            world.DestroyGameObject(object);
            userLights.RemoveAt(i);
            return;
        }
    }
}

void SceneEditorContentModel::ClearManualObjects(GameWorld& world) noexcept {
    for (std::size_t i = 0; i < placed.GetSize(); ++i) {
        GameObject* object = placed[i];
        if (object != nullptr && object->GetSceneInstanceId() == kInvalidSceneInstanceId) {
            world.DestroyGameObject(object);
        }
    }
    for (std::size_t i = 0; i < userLights.GetSize(); ++i) {
        GameObject* object = userLights[i];
        if (object != nullptr && object->GetSceneInstanceId() == kInvalidSceneInstanceId) {
            world.DestroyGameObject(object);
        }
    }
}

void SceneEditorContentModel::ClearLists() noexcept {
    placed.Clear();
    placedRel.Clear();
    userLights.Clear();
}

bool SceneEditorContentModel::ShouldCapture(const GameObject* object) const noexcept {
    if (object == nullptr) {
        return false;
    }
    for (std::size_t i = 0; i < placed.GetSize(); ++i) {
        if (placed[i] == object) {
            return true;
        }
    }
    for (std::size_t i = 0; i < userLights.GetSize(); ++i) {
        if (userLights[i] == object) {
            return true;
        }
    }
    return false;
}

SceneCaptureContext SceneEditorContentModel::BuildCaptureContext() const noexcept {
    static CaptureUserData userData{};
    userData.model = this;
    SceneCaptureContext context{};
    context.meshAssetUserData = const_cast<CaptureUserData*>(&userData);
    context.textureUserData = const_cast<CaptureUserData*>(&userData);
    context.resolveMeshAssetPath = ResolveMeshAssetPath;
    context.resolveTexturePath = ResolveTexturePath;
    return context;
}

Utf8String SceneEditorContentModel::LookupMeshAssetRel(const GameObject& owner) const noexcept {
    if (const GameObject* placedOwner = FindPlacedOwner(const_cast<GameObject*>(&owner))) {
        for (std::size_t i = 0; i < placed.GetSize(); ++i) {
            if (placed[i] == placedOwner) {
                return placedRel[i];
            }
        }
    }
    return {};
}

GameObject* SceneEditorContentModel::FindPlacedOwner(GameObject* object) const noexcept {
    while (object != nullptr) {
        for (std::size_t i = 0; i < placed.GetSize(); ++i) {
            if (placed[i] == object) {
                return object;
            }
        }
        object = object->GetParent();
    }
    return nullptr;
}

Utf8String SceneEditorContentModel::LookupTextureAssetRel(const GameObject& owner) const noexcept {
    const MaterialComponent* material = owner.GetComponent<MaterialComponent>();
    if (material == nullptr || !material->GetBaseColorTexture()) {
        return {};
    }
    const Utf8String& rel = LookupMeshAssetRel(owner);
    if (rel.IsEmpty() || std::strcmp(rel.CStr(), "builtin:unit_cube") == 0) {
        return {};
    }
    return rel;
}

void SceneEditorContentModel::BindRole(
        GameObject* object,
        const SceneEntityRoleInfo roleInfo,
        const SceneEditorContentBindingHooks& hooks) noexcept {
    if (object == nullptr) {
        return;
    }
    switch (roleInfo.role) {
    case SceneEntityRole::Light:
        TrackRoot(object);
        TrackUserLight(object);
        if (object->GetComponent<MeshComponent>() == nullptr && hooks.unitCubeAsset != nullptr && *hooks.unitCubeAsset) {
            object->AddComponent<MeshComponent>(
                    *hooks.unitCubeAsset, SceneMeshSlot::UnitCube, Vector3{1.0F, 1.0F, 1.0F});
            if (MaterialComponent* material = object->AddComponent<MaterialComponent>()) {
                material->SetMetallic(0.12F);
                material->SetRoughness(0.35F);
                if (const PointLightComponent* pointLight = object->GetComponent<PointLightComponent>()) {
                    material->SetEmissive(pointLight->GetColor(), 7.5F);
                } else if (const SpotLightComponent* spotLight = object->GetComponent<SpotLightComponent>()) {
                    material->SetEmissive(spotLight->GetColor(), 7.5F);
                }
            }
            if (hooks.syncLightGizmoEmissive != nullptr) {
                hooks.syncLightGizmoEmissive(object);
            }
        }
        break;
    case SceneEntityRole::Mesh:
        TrackRoot(object);
        TrackPlaced(object, roleInfo.meshAssetPath);
        break;
    case SceneEntityRole::SpawnPoint:
        TrackRoot(object);
        TrackPlaced(object, Utf8String{});
        break;
    case SceneEntityRole::None:
    default:
        break;
    }
}

void SceneEditorContentModel::IntegrateLoadedInstance(
        GameWorld& world,
        const SceneInstanceId instanceId,
        const SceneDocument& document,
        const SceneEditorContentBindingHooks& hooks) noexcept {
    const Array<GameObject*> instanceObjects =
            SceneEntityRoleClassifier::CollectInstanceObjects(world, instanceId);

    for (std::size_t ei = 0; ei < document.entities.GetSize() && ei < instanceObjects.GetSize(); ++ei) {
        GameObject* object = instanceObjects[ei];
        if (object == nullptr) {
            continue;
        }
        const SceneEntityRoleInfo roleInfo = SceneEntityRoleClassifier::Classify(document.entities[ei]);
        BindRole(object, roleInfo, hooks);
    }
}

namespace {

void CollectSubtreeForContentModel(const GameObject& node, Array<const GameObject*>& out) {
    out.PushBack(&node);
    const Array<GameObject*>& children = node.GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            CollectSubtreeForContentModel(*children[i], out);
        }
    }
}

}  // namespace

void SceneEditorContentModel::IntegrateSubtree(GameObject& root) noexcept {
    Array<const GameObject*> subtree;
    CollectSubtreeForContentModel(root, subtree);

    for (std::size_t i = 0; i < subtree.GetSize(); ++i) {
        GameObject* object = const_cast<GameObject*>(subtree[i]);
        if (object == nullptr) {
            continue;
        }
        TrackRoot(object);
        if (object->GetComponent<MeshComponent>() != nullptr) {
            TrackPlaced(object, LookupMeshAssetRel(*object));
        }
        if (object->GetComponent<PointLightComponent>() != nullptr || object->GetComponent<SpotLightComponent>() != nullptr) {
            TrackUserLight(object);
        }
    }
}

void SceneEditorContentModel::UntrackSubtree(const GameObject& root) noexcept {
    Array<const GameObject*> subtree;
    CollectSubtreeForContentModel(root, subtree);

    const auto contains = [&subtree](const GameObject* candidate) {
        for (std::size_t i = 0; i < subtree.GetSize(); ++i) {
            if (subtree[i] == candidate) {
                return true;
            }
        }
        return false;
    };

    for (std::size_t i = roots.GetSize(); i > 0; --i) {
        if (contains(roots[i - 1U])) {
            roots.RemoveAt(i - 1U);
        }
    }
    for (std::size_t i = placed.GetSize(); i > 0; --i) {
        if (contains(placed[i - 1U])) {
            placed.RemoveAt(i - 1U);
            if (i - 1U < placedRel.GetSize()) {
                placedRel.RemoveAt(i - 1U);
            }
        }
    }
    for (std::size_t i = userLights.GetSize(); i > 0; --i) {
        if (contains(userLights[i - 1U])) {
            userLights.RemoveAt(i - 1U);
        }
    }
}

}  // namespace Spark
