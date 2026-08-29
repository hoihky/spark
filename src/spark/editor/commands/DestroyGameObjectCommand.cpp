#include "spark/editor/commands/DestroyGameObjectCommand.hpp"

#include "spark/editor/EditorSelection.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"

namespace Spark::Editor {

DestroyGameObjectCommand::DestroyGameObjectCommand(
        GameWorld& inWorld,
        SceneEditorContentModel& inContent,
        EditorSelection& inSelection,
        GameObject& target)
    : world(&inWorld),
      content(&inContent),
      selection(&inSelection),
      captureCtx(inContent.BuildCaptureContext()),
      initialLiveId(target.GetId()),
      selectedObjectId(inSelection.GetPrimary() != nullptr ? inSelection.GetPrimary()->GetId() : 0),
      targetName(target.GetName()) {
    snapshot = HierarchySceneSnapshot::CaptureSubtree(inWorld, target, captureCtx);
    rootEntityId = HierarchySceneSnapshot::FindRootEntityId(snapshot, target);
}

void DestroyGameObjectCommand::DestroyTarget() {
    if (world == nullptr || content == nullptr) {
        return;
    }
    const std::uint64_t liveId = restoredLiveId != 0 ? restoredLiveId : initialLiveId;
    GameObject* target = world->FindGameObjectById(liveId);
    if (target == nullptr) {
        return;
    }
    if (selection != nullptr && selection->GetPrimary() != nullptr) {
        Array<const GameObject*> subtree;
        HierarchySceneSnapshot::CollectSubtree(*target, subtree);
        for (std::size_t i = 0; i < subtree.GetSize(); ++i) {
            if (subtree[i] == selection->GetPrimary()) {
                selection->Clear();
                break;
            }
        }
    }
    content->UntrackSubtree(*target);
    world->DestroyGameObject(target);
}

void DestroyGameObjectCommand::RestoreTarget() {
    if (world == nullptr || content == nullptr) {
        return;
    }
    HierarchyRestoreResult restored{};
    SceneApplyContext applyCtx{};
    if (!HierarchySceneSnapshot::RestoreSubtree(snapshot, *world, applyCtx, rootEntityId, restored)) {
        return;
    }
    if (restored.root != nullptr) {
        restoredLiveId = restored.root->GetId();
        content->IntegrateSubtree(*restored.root);
        if (selection != nullptr) {
            selection->SetPrimary(restored.root);
        }
        return;
    }
    if (selection != nullptr && selectedObjectId != 0) {
        if (GameObject* previous = world->FindGameObjectById(selectedObjectId)) {
            selection->SetPrimary(previous);
        }
    }
}

void DestroyGameObjectCommand::Undo() {
    RestoreTarget();
}

void DestroyGameObjectCommand::Redo() {
    DestroyTarget();
}

Utf8String DestroyGameObjectCommand::GetDescription() const {
    Utf8String desc = Utf8String("Delete ");
    desc.AppendUtf8(targetName.CStr());
    return desc;
}

}  // namespace Spark::Editor
