#pragma once

#include "spark/editor/IEditorPanel.hpp"
#include "spark/gui/controls/TreeView.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

class GameObject;
class GameWorld;

namespace Editor {

class EditorSelection;

class HierarchyPanel final : public IEditorPanel {
public:
    HierarchyPanel();
    ~HierarchyPanel() override = default;

    [[nodiscard]] Utf8String GetPanelId() const override { return Utf8String("hierarchy"); }
    [[nodiscard]] Utf8String GetDisplayName() const override { return Utf8String("Hierarchy"); }
    [[nodiscard]] Gui::Widget* GetRootWidget() noexcept override { return root.Get(); }
    [[nodiscard]] UniquePtr<Gui::Widget> ReleaseRootWidget() { return MoveTemp(root); }

    void OnAttach(EditorContext& ctx) override;
    void OnTick(const FrameTiming& timing, EditorContext& ctx) override;

private:
    void RebuildTree();
    void OnTreeSelection(int nodeId);
    [[nodiscard]] std::uint32_t ComputeSceneRevision() const noexcept;

    UniquePtr<Gui::Widget> root;
    Gui::TreeView* tree = nullptr;
    GameWorld* world = nullptr;
    EditorSelection* selection = nullptr;
    Utf8String* statusLine = nullptr;
    Array<GameObject*> nodeObjects;
    std::uint32_t sceneRevision = 0;
    bool needsTreeRebuild = true;
};

}  // namespace Editor
}  // namespace Spark
