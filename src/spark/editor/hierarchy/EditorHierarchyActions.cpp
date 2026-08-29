#include "spark/editor/hierarchy/EditorHierarchyActions.hpp"

#include <cstdio>

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/editor/EditorSelection.hpp"
#include "spark/editor/commands/CreateGameObjectCommand.hpp"
#include "spark/editor/commands/DestroyGameObjectCommand.hpp"
#include "spark/editor/commands/DuplicateGameObjectCommand.hpp"
#include "spark/editor/commands/SetGameObjectParentCommand.hpp"
#include "spark/editor/hierarchy/HierarchySceneSnapshot.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"

namespace Spark::Editor {

namespace {

Utf8String MakeUniqueEmptyName(const GameWorld& world) {
    for (int attempt = 0; attempt < 128; ++attempt) {
        Utf8String candidate = Utf8String("GameObject");
        if (attempt > 0) {
            candidate.AppendUtf8(" ");
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%d", attempt);
            candidate.AppendUtf8(buf);
        }
        bool taken = false;
        world.ForEachGameObject([&](const GameObject* object) {
            if (object != nullptr && object->GetName() == candidate) {
                taken = true;
            }
        });
        if (!taken) {
            return candidate;
        }
    }
    return Utf8String("GameObject");
}

}  // namespace

bool EditorHierarchyActions::CanEdit(const EditorContext& ctx) noexcept {
    return ctx.world != nullptr && ctx.contentModel != nullptr && ctx.commandStack != nullptr
            && ctx.selection != nullptr && ctx.mode == EditorMode::Edit;
}

GameObject* EditorHierarchyActions::ResolveContextTarget(
        const EditorContext& ctx,
        GameObject* contextObject) noexcept {
    if (contextObject != nullptr) {
        return contextObject;
    }
    if (ctx.selection != nullptr) {
        return ctx.selection->GetPrimary();
    }
    return nullptr;
}

void EditorHierarchyActions::CreateEmpty(EditorContext& ctx, GameObject* parent) {
    if (!CanEdit(ctx)) {
        return;
    }
    const Utf8String name = MakeUniqueEmptyName(*ctx.world);
    auto command = MakeUnique<CreateGameObjectCommand>(*ctx.world, *ctx.contentModel, name, parent);
    CreateGameObjectCommand* raw = command.Get();
    ctx.commandStack->Execute(UniquePtr<IEditorCommand>(command.Release()));
    if (raw != nullptr && raw->GetCreatedObject() != nullptr) {
        ctx.selection->SetPrimary(raw->GetCreatedObject());
    }
    ctx.statusLine = Utf8String("Created empty GameObject.");
}

void EditorHierarchyActions::DeleteObject(EditorContext& ctx, GameObject& target) {
    if (!CanEdit(ctx) || HierarchySceneSnapshot::IsEditorChrome(&target)) {
        return;
    }
    auto command =
            MakeUnique<DestroyGameObjectCommand>(*ctx.world, *ctx.contentModel, *ctx.selection, target);
    ctx.commandStack->Execute(UniquePtr<IEditorCommand>(command.Release()));
    ctx.statusLine = Utf8String("Deleted GameObject.");
}

void EditorHierarchyActions::DuplicateObject(EditorContext& ctx, GameObject& source) {
    if (!CanEdit(ctx) || HierarchySceneSnapshot::IsEditorChrome(&source)) {
        return;
    }
    auto command =
            MakeUnique<DuplicateGameObjectCommand>(*ctx.world, *ctx.contentModel, *ctx.selection, source);
    ctx.commandStack->Execute(UniquePtr<IEditorCommand>(command.Release()));
    ctx.statusLine = Utf8String("Duplicated GameObject.");
}

void EditorHierarchyActions::ReparentToRoot(EditorContext& ctx, GameObject& child) {
    if (!CanEdit(ctx) || HierarchySceneSnapshot::IsEditorChrome(&child) || child.GetParent() == nullptr) {
        return;
    }
    auto command = MakeUnique<SetGameObjectParentCommand>(*ctx.world, child, nullptr);
    ctx.commandStack->Execute(UniquePtr<IEditorCommand>(command.Release()));
    ctx.statusLine = Utf8String("Reparented to scene root.");
}

}  // namespace Spark::Editor
