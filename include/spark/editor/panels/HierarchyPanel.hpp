#pragma once

#include "spark/editor/EditorContext.hpp"
#include "spark/editor/IEditorPanel.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/controls/IUiControls.hpp"

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
    [[nodiscard]] Ui::IUiElement* GetRootElement() noexcept override { return root.Get(); }
    [[nodiscard]] UniquePtr<Ui::IUiElement> ReleaseRootElement() override { return MoveTemp(root); }

    void OnAttach(EditorContext& ctx) override;
    void OnTick(const FrameTiming& timing, EditorContext& ctx) override;

    /** Highlights the tree row for the current <c>EditorSelection</c> primary (viewport → hierarchy sync). */
    void SyncToSelection();

private:
    void EnsureBuilt();
    void RebuildTree();
    void OnTreeSelection(int nodeId);
    void OnTreeContextMenu(int nodeId);
    void HandleShortcuts(EditorContext& ctx);
    void OpenContextMenu(EditorContext& ctx, float menuX, float menuY, GameObject* contextObject);
    [[nodiscard]] GameObject* ResolveNodeObject(int nodeId) const noexcept;
    [[nodiscard]] std::uint32_t ComputeSceneRevision() const noexcept;

    static void OnCreateClicked(void* userData) noexcept;
    static void OnDuplicateClicked(void* userData) noexcept;
    static void OnDeleteClicked(void* userData) noexcept;

    UniquePtr<Ui::IUiElement> root;
    Ui::ITreeView* tree = nullptr;
    EditorContext panelContext{};
    GameWorld* world = nullptr;
    EditorSelection* selection = nullptr;
    Utf8String* statusLine = nullptr;
    Array<GameObject*> nodeObjects;
    std::uint32_t sceneRevision = 0;
    bool needsTreeRebuild = true;
    bool suppressSelectionCallback = false;
    bool built = false;
    GameObject* lastSyncedSelection = nullptr;
};

}  // namespace Editor
}  // namespace Spark
