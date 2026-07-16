#include "spark/editor/panels/HierarchyPanel.hpp"

#include "spark/editor/EditorSelection.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/gui/controls/Label.hpp"
#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/controls/StackPanel.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark::Editor {

HierarchyPanel::HierarchyPanel() {
    auto shell = MakeUnique<Gui::Panel>();
    shell->SetPadding(6.0F);

    auto stack = MakeUnique<Gui::StackPanel>();
    stack->SetSpacing(4.0F);

    auto treeUp = MakeUnique<Gui::TreeView>();
    treeUp->SetRowHeight(26.0F);
    treeUp->SetItemFontSize(18.0F);
    treeUp->SetOnSelectionChanged([this](const int nodeId) { OnTreeSelection(nodeId); });
    tree = treeUp.Get();
    stack->AddChild(MoveTemp(treeUp));

    shell->AddChild(MoveTemp(stack));
    root.Reset(shell.Release());
}

void HierarchyPanel::OnAttach(EditorContext& ctx) {
    world = ctx.world;
    selection = ctx.selection;
    statusLine = &ctx.statusLine;
    needsTreeRebuild = true;
}

void HierarchyPanel::OnTick(const FrameTiming& /*timing*/, EditorContext& /*ctx*/) {
    if (tree == nullptr || world == nullptr) {
        return;
    }
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

void HierarchyPanel::OnTreeSelection(const int nodeId) {
    if (selection == nullptr || nodeId < 0 || static_cast<std::size_t>(nodeId) >= nodeObjects.GetSize()) {
        return;
    }
    GameObject* obj = nodeObjects[static_cast<std::size_t>(nodeId)];
    selection->SetPrimary(obj);
    if (statusLine != nullptr && obj != nullptr) {
        *statusLine = obj->GetName();
    }
}

}  // namespace Spark::Editor
