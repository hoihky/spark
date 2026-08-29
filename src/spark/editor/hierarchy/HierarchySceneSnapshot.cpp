#include "spark/editor/hierarchy/HierarchySceneSnapshot.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"

namespace Spark::Editor {

namespace {

void CollectSubtreeImpl(const GameObject& root, Array<const GameObject*>& out) {
    out.PushBack(&root);
    const Array<GameObject*>& children = root.GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            CollectSubtreeImpl(*children[i], out);
        }
    }
}

}  // namespace

bool HierarchySceneSnapshot::IsEditorChrome(const GameObject* object) noexcept {
    if (object == nullptr) {
        return true;
    }
    const Utf8String& name = object->GetName();
    return name == Utf8String("EditorGui") || name == Utf8String("EditorStatusHud");
}

void HierarchySceneSnapshot::CollectSubtree(const GameObject& root, Array<const GameObject*>& out) {
    CollectSubtreeImpl(root, out);
}

SceneDocument HierarchySceneSnapshot::CaptureSubtree(
        const GameWorld& world,
        const GameObject& root,
        const SceneCaptureContext& ctx) {
    Array<const GameObject*> subtree;
    CollectSubtree(root, subtree);

    const auto includeEntity = [&subtree](const GameObject* candidate) {
        if (candidate == nullptr) {
            return false;
        }
        for (std::size_t i = 0; i < subtree.GetSize(); ++i) {
            if (subtree[i] == candidate) {
                return true;
            }
        }
        return false;
    };

    SceneSerializer serializer;
    return serializer.Capture(world, ctx, includeEntity);
}

bool HierarchySceneSnapshot::RestoreSubtree(
        const SceneDocument& document,
        GameWorld& world,
        const SceneApplyContext& ctx,
        const std::uint64_t rootEntityId,
        HierarchyRestoreResult& out) {
    out.root = nullptr;
    out.idToObject.Clear();
    HashMap<std::uint64_t, GameObject*> idMap;
    SceneDeserializer deserializer;
    if (!deserializer.Apply(document, world, ctx, &idMap)) {
        return false;
    }
    out.idToObject = MoveTemp(idMap);
    if (rootEntityId != 0) {
        if (GameObject* const* found = out.idToObject.Find(rootEntityId)) {
            out.root = *found;
        }
    }
    return out.root != nullptr;
}

std::uint64_t HierarchySceneSnapshot::FindRootEntityId(
        const SceneDocument& document,
        const GameObject& root) noexcept {
    const std::uint64_t liveId = root.GetId();
    for (std::size_t i = 0; i < document.entities.GetSize(); ++i) {
        if (document.entities[i].id == liveId) {
            return liveId;
        }
    }
    if (!document.entities.IsEmpty()) {
        return document.entities[0].id;
    }
    return 0;
}

}  // namespace Spark::Editor
