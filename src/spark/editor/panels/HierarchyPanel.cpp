#include "spark/editor/panels/HierarchyPanel.hpp"

#include "spark/editor/EditorSelection.hpp"
#include "spark/editor/hierarchy/EditorHierarchyActions.hpp"
#include "spark/editor/hierarchy/HierarchySceneSnapshot.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/ui/Ui.hpp"
#include "spark/ui/runtime/UiContextMenu.hpp"
#include "spark/ui/spark/UiChild.hpp"

#include <GLFW/glfw3.h>

namespace Spark::Editor {

HierarchyPanel::HierarchyPanel() = default;

void HierarchyPanel::EnsureBuilt() {
    if (built) {
        return;
    }
    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::PanelDesc shellDesc{};
    shellDesc.id = Utf8String("hierarchy_shell");
    shellDesc.title = Utf8String("Hierarchy");
    auto shell = factory.CreatePanel(shellDesc);

    Ui::ButtonDesc createDesc{};
    createDesc.id = Utf8String("hierarchy_create");
    createDesc.label = Utf8String("+ Empty");
    auto createBtn = factory.CreateButton(createDesc);
    Ui::UiVoidCallback createCb{};
    createCb.fn = &HierarchyPanel::OnCreateClicked;
    createCb.userData = this;
    createBtn->SetOnClick(createCb);
    AdoptUiChild(*shell, MoveTemp(createBtn));

    Ui::ButtonDesc duplicateDesc{};
    duplicateDesc.id = Utf8String("hierarchy_duplicate");
    duplicateDesc.label = Utf8String("Duplicate");
    auto duplicateBtn = factory.CreateButton(duplicateDesc);
    Ui::UiVoidCallback duplicateCb{};
    duplicateCb.fn = &HierarchyPanel::OnDuplicateClicked;
    duplicateCb.userData = this;
    duplicateBtn->SetOnClick(duplicateCb);
    AdoptUiChild(*shell, MoveTemp(duplicateBtn));

    Ui::ButtonDesc deleteDesc{};
    deleteDesc.id = Utf8String("hierarchy_delete");
    deleteDesc.label = Utf8String("Delete");
    auto deleteBtn = factory.CreateButton(deleteDesc);
    Ui::UiVoidCallback deleteCb{};
    deleteCb.fn = &HierarchyPanel::OnDeleteClicked;
    deleteCb.userData = this;
    deleteBtn->SetOnClick(deleteCb);
    AdoptUiChild(*shell, MoveTemp(deleteBtn));

    Ui::TreeViewDesc treeDesc{};
    treeDesc.id = Utf8String("hierarchy_tree");
    treeDesc.rowHeight = 26.0F;
    treeDesc.itemFontSize = 18.0F;
    auto treeUp = factory.CreateTreeView(treeDesc);
    tree = treeUp.Get();
    Ui::UiIntCallback selectCb{};
    selectCb.fn = [](void* userData, const int nodeId) {
        static_cast<HierarchyPanel*>(userData)->OnTreeSelection(nodeId);
    };
    selectCb.userData = this;
    tree->SetOnSelectionChanged(selectCb);
    Ui::UiIntCallback menuCb{};
    menuCb.fn = [](void* userData, const int nodeId) {
        static_cast<HierarchyPanel*>(userData)->OnTreeContextMenu(nodeId);
    };
    menuCb.userData = this;
    tree->SetOnNodeContextMenu(menuCb);
    AdoptUiChild(*shell, MoveTemp(treeUp));

    root.Reset(static_cast<Ui::IUiElement*>(shell.Release()));
    built = true;
}

void HierarchyPanel::OnAttach(EditorContext& ctx) {
    EnsureBuilt();
    panelContext = ctx;
    world = ctx.world;
    selection = ctx.selection;
    statusLine = &ctx.statusLine;
    needsTreeRebuild = true;
}

void HierarchyPanel::OnTick(const FrameTiming& /*timing*/, EditorContext& ctx) {
    panelContext = ctx;
    world = ctx.world;
    selection = ctx.selection;
    statusLine = &ctx.statusLine;

    if (tree == nullptr || world == nullptr) {
        return;
    }

    HandleShortcuts(ctx);

    const std::uint32_t revision = ComputeSceneRevision();
    if (!needsTreeRebuild && revision == sceneRevision) {
        return;
    }
    RebuildTree();
    needsTreeRebuild = false;
    sceneRevision = revision;
}

std::uint32_t HierarchyPanel::ComputeSceneRevision() const noexcept {
    std::uint32_t rev = 0;
    world->ForEachGameObject([&](GameObject* obj) {
        if (obj == nullptr) {
            return;
        }
        const Utf8String& name = obj->GetName();
        if (name == Utf8String("EditorGui") || name == Utf8String("EditorStatusHud")) {
            return;
        }
        rev ^= static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(obj) >> 4U);
        rev = rev * 16777619U
                + static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(obj->GetParent()) >> 4U);
        for (const char* p = name.CStr(); *p != '\0'; ++p) {
            rev = rev * 31U + static_cast<unsigned char>(*p);
        }
    });
    return rev;
}

void HierarchyPanel::RebuildTree() {
    tree->Clear();
    nodeObjects.Clear();

    Array<GameObject*> allObjects;
    world->ForEachGameObject([&](GameObject* obj) {
        if (obj == nullptr) {
            return;
        }
        const Utf8String& name = obj->GetName();
        if (name == Utf8String("EditorGui") || name == Utf8String("EditorStatusHud")) {
            return;
        }
        allObjects.PushBack(obj);
    });

    Array<int> objectToNode;
    objectToNode.Resize(allObjects.GetSize());
    for (std::size_t i = 0; i < objectToNode.GetSize(); ++i) {
        objectToNode[i] = -1;
    }

    auto findIndex = [&](const GameObject* obj) -> int {
        for (std::size_t i = 0; i < allObjects.GetSize(); ++i) {
            if (allObjects[i] == obj) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    bool progress = true;
    while (progress) {
        progress = false;
        for (std::size_t i = 0; i < allObjects.GetSize(); ++i) {
            if (objectToNode[i] >= 0) {
                continue;
            }
            GameObject* obj = allObjects[i];
            GameObject* parent = obj->GetParent();
            int parentNode = -1;
            if (parent != nullptr) {
                const int parentIdx = findIndex(parent);
                if (parentIdx < 0 || objectToNode[static_cast<std::size_t>(parentIdx)] < 0) {
                    continue;
                }
                parentNode = objectToNode[static_cast<std::size_t>(parentIdx)];
            }
            const int nodeId = tree->AddItem(parentNode, obj->GetName());
            if (nodeId >= 0) {
                objectToNode[i] = nodeId;
                while (static_cast<std::size_t>(nodeId) >= nodeObjects.GetSize()) {
                    nodeObjects.PushBack(nullptr);
                }
                nodeObjects[static_cast<std::size_t>(nodeId)] = obj;
                progress = true;
            }
        }
    }
}

GameObject* HierarchyPanel::ResolveNodeObject(const int nodeId) const noexcept {
    if (nodeId < 0 || static_cast<std::size_t>(nodeId) >= nodeObjects.GetSize()) {
        return nullptr;
    }
    return nodeObjects[static_cast<std::size_t>(nodeId)];
}

void HierarchyPanel::OnTreeSelection(const int nodeId) {
    if (suppressSelectionCallback) {
        return;
    }
    GameObject* obj = ResolveNodeObject(nodeId);
    if (selection == nullptr || obj == nullptr) {
        return;
    }
    selection->SetPrimary(obj);
    if (statusLine != nullptr) {
        *statusLine = obj->GetName();
    }
}

void HierarchyPanel::OnTreeContextMenu(const int nodeId) {
    GameObject* obj = ResolveNodeObject(nodeId);
    if (obj == nullptr || panelContext.engine == nullptr) {
        return;
    }
    IInput& input = panelContext.engine->GetInput();
    float mx = 0.0F;
    float my = 0.0F;
    input.GetCursorFramebufferPixels(mx, my);
    OpenContextMenu(panelContext, mx, my, obj);
}

void HierarchyPanel::OpenContextMenu(
        EditorContext& ctx,
        const float menuX,
        const float menuY,
        GameObject* contextObject) {
    if (!EditorHierarchyActions::CanEdit(ctx)) {
        return;
    }

    Array<Utf8String> labels;
    labels.PushBack(Utf8String("Create Empty Child"));
    labels.PushBack(Utf8String("Duplicate"));
    labels.PushBack(Utf8String("Delete"));
    if (contextObject != nullptr && contextObject->GetParent() != nullptr) {
        labels.PushBack(Utf8String("Unparent"));
    }

    HierarchyPanel* self = this;
    Ui::GetUiContextMenu().Open(
            menuX,
            menuY,
            MoveTemp(labels),
            [self, contextObject](const int index) {
                EditorContext& ctx = self->panelContext;
                switch (index) {
                case 0:
                    EditorHierarchyActions::CreateEmpty(ctx, contextObject);
                    break;
                case 1:
                    if (contextObject != nullptr) {
                        EditorHierarchyActions::DuplicateObject(ctx, *contextObject);
                    }
                    break;
                case 2:
                    if (contextObject != nullptr) {
                        EditorHierarchyActions::DeleteObject(ctx, *contextObject);
                    }
                    break;
                case 3:
                    if (contextObject != nullptr) {
                        EditorHierarchyActions::ReparentToRoot(ctx, *contextObject);
                    }
                    break;
                default:
                    break;
                }
                self->needsTreeRebuild = true;
            });
}

void HierarchyPanel::HandleShortcuts(EditorContext& ctx) {
    if (!EditorHierarchyActions::CanEdit(ctx) || ctx.engine == nullptr) {
        return;
    }
    IInput& input = ctx.engine->GetInput();
    const bool ctrlDown = input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
    GameObject* primary = ctx.selection != nullptr ? ctx.selection->GetPrimary() : nullptr;

    if (input.IsKeyPressedThisFrame(GLFW_KEY_DELETE) && primary != nullptr) {
        EditorHierarchyActions::DeleteObject(ctx, *primary);
        needsTreeRebuild = true;
        return;
    }
    if (ctrlDown && input.IsKeyPressedThisFrame(GLFW_KEY_D) && primary != nullptr) {
        EditorHierarchyActions::DuplicateObject(ctx, *primary);
        needsTreeRebuild = true;
    }
}

void HierarchyPanel::SyncToSelection() {
    if (tree == nullptr || selection == nullptr) {
        return;
    }
    GameObject* primary = selection->GetPrimary();
    suppressSelectionCallback = true;
    if (primary == nullptr) {
        tree->SetSelectedNodeId(-1);
        suppressSelectionCallback = false;
        return;
    }
    for (std::size_t i = 0; i < nodeObjects.GetSize(); ++i) {
        if (nodeObjects[i] == primary) {
            tree->SetSelectedNodeId(static_cast<int>(i));
            suppressSelectionCallback = false;
            return;
        }
    }
    needsTreeRebuild = true;
    suppressSelectionCallback = false;
}

void HierarchyPanel::OnCreateClicked(void* userData) noexcept {
    auto* panel = static_cast<HierarchyPanel*>(userData);
    if (panel == nullptr) {
        return;
    }
    GameObject* parent = EditorHierarchyActions::ResolveContextTarget(
            panel->panelContext, panel->selection != nullptr ? panel->selection->GetPrimary() : nullptr);
    EditorHierarchyActions::CreateEmpty(panel->panelContext, parent);
    panel->needsTreeRebuild = true;
}

void HierarchyPanel::OnDuplicateClicked(void* userData) noexcept {
    auto* panel = static_cast<HierarchyPanel*>(userData);
    if (panel == nullptr || panel->selection == nullptr) {
        return;
    }
    GameObject* primary = panel->selection->GetPrimary();
    if (primary == nullptr) {
        return;
    }
    EditorHierarchyActions::DuplicateObject(panel->panelContext, *primary);
    panel->needsTreeRebuild = true;
}

void HierarchyPanel::OnDeleteClicked(void* userData) noexcept {
    auto* panel = static_cast<HierarchyPanel*>(userData);
    if (panel == nullptr || panel->selection == nullptr) {
        return;
    }
    GameObject* primary = panel->selection->GetPrimary();
    if (primary == nullptr) {
        return;
    }
    EditorHierarchyActions::DeleteObject(panel->panelContext, *primary);
    panel->needsTreeRebuild = true;
}

}  // namespace Spark::Editor
