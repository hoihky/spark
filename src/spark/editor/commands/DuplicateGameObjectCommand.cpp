#include "spark/editor/commands/DuplicateGameObjectCommand.hpp"

#include "spark/editor/EditorSelection.hpp"
#include "spark/editor/hierarchy/HierarchySceneSnapshot.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"

namespace Spark::Editor {

namespace {

void AppendCopySuffix(SceneDocument& document, const std::uint64_t rootEntityId) {
    for (std::size_t i = 0; i < document.entities.GetSize(); ++i) {
        if (document.entities[i].id == rootEntityId) {
            document.entities[i].name.AppendUtf8(" Copy");
            return;
        }
    }
}

}  // namespace

DuplicateGameObjectCommand::DuplicateGameObjectCommand(
        GameWorld& inWorld,
        SceneEditorContentModel& inContent,
        EditorSelection& inSelection,
        GameObject& source)
    : world(&inWorld),
      content(&inContent),
      selection(&inSelection),
      captureCtx(inContent.BuildCaptureContext()),
      parentObjectId(source.GetParent() != nullptr ? source.GetParent()->GetId() : 0),
      sourceName(source.GetName()) {
    templateDocument = HierarchySceneSnapshot::CaptureSubtree(inWorld, source, captureCtx);
    rootEntityId = HierarchySceneSnapshot::FindRootEntityId(templateDocument, source);
    AppendCopySuffix(templateDocument, rootEntityId);
}

void DuplicateGameObjectCommand::CreateDuplicate() {
    if (world == nullptr || content == nullptr || rootEntityId == 0) {
        return;
    }
    HierarchyRestoreResult restored{};
    SceneApplyContext applyCtx{};
    if (!HierarchySceneSnapshot::RestoreSubtree(templateDocument, *world, applyCtx, rootEntityId, restored)) {
        return;
    }
    if (restored.root == nullptr) {
        return;
    }
    duplicateLiveId = restored.root->GetId();
    GameObject* parent = parentObjectId != 0 ? world->FindGameObjectById(parentObjectId) : nullptr;
    (void)world->SetParent(restored.root, parent);
    content->IntegrateSubtree(*restored.root);
    if (selection != nullptr) {
        selection->SetPrimary(restored.root);
    }
}

void DuplicateGameObjectCommand::DestroyDuplicate() {
    if (world == nullptr || content == nullptr || duplicateLiveId == 0) {
        return;
    }
    GameObject* duplicate = world->FindGameObjectById(duplicateLiveId);
    if (duplicate == nullptr) {
        duplicateLiveId = 0;
        return;
    }
    content->UntrackSubtree(*duplicate);
    world->DestroyGameObject(duplicate);
    duplicateLiveId = 0;
}

void DuplicateGameObjectCommand::Undo() {
    DestroyDuplicate();
}

void DuplicateGameObjectCommand::Redo() {
    CreateDuplicate();
}

Utf8String DuplicateGameObjectCommand::GetDescription() const {
    Utf8String desc = Utf8String("Duplicate ");
    desc.AppendUtf8(sourceName.CStr());
    return desc;
}

}  // namespace Spark::Editor
