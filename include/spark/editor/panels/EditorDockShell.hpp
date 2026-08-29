#pragma once

#include "spark/editor/EditorDefaultLayout.hpp"
#include "spark/editor/IEditorPanel.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiTypes.hpp"

#include "spark/ui/runtime/EditorLayoutStore.hpp"

namespace Spark {

class Utf8String;

namespace Editor {

class EditorViewport;

struct EditorGizmoToolbarBinding {
    EditorViewport* viewport = nullptr;
    Utf8String* statusLine = nullptr;
};

/**
 * Top-level editor chrome: toolbar + <c>SparkDockWorkspace</c> (collapsible left/right panels).
 */
class EditorDockShell {
public:
    void SetSidebarWidth(float widthPx) noexcept;
    [[nodiscard]] float GetSidebarWidth() const noexcept { return sidebarWidthPx; }
    [[nodiscard]] float GetRightPanelWidth() const noexcept;

    void ToggleLeftPanel() noexcept;
    void ToggleRightPanel() noexcept;

    void SetPanels(
            UniquePtr<Ui::IUiElement> hierarchyRoot,
            UniquePtr<Ui::IUiElement> projectRoot,
            UniquePtr<Ui::IUiElement> inspectorRoot);

    void SetGizmoBinding(EditorViewport* viewportIn, Utf8String* statusLineIn) noexcept;
    void SyncGizmoToolbar() noexcept;

    void ApplyLayout(const Ui::SceneEditorLayoutSettings& layout) noexcept;
    void SyncLayout(const Ui::Rect& viewport) noexcept;

    [[nodiscard]] Ui::IUiElement* GetRootElement() noexcept { return root.Get(); }
    [[nodiscard]] UniquePtr<Ui::IUiElement> ReleaseRootElement() { return MoveTemp(root); }
    /** 3D viewport region in framebuffer pixels (center pane after layout). */
    [[nodiscard]] Ui::Rect GetWorldViewportRect() const noexcept;

private:
    void Rebuild(
            UniquePtr<Ui::IUiElement> hierarchyRoot,
            UniquePtr<Ui::IUiElement> projectRoot,
            UniquePtr<Ui::IUiElement> inspectorRoot);

    void* dockHost = nullptr;
    float sidebarWidthPx = EditorDefaultLayout::kLeftWidthPx;
    float rightPanelWidthPx = EditorDefaultLayout::kRightWidthPx;
    float leftStackSplit = EditorDefaultLayout::kLeftStackSplit;
    UniquePtr<Ui::IUiElement> root;
    EditorViewport* gizmoViewport = nullptr;
    Utf8String* gizmoStatusLine = nullptr;
    Ui::IButton* gizmoMoveButton = nullptr;
    Ui::IButton* gizmoRotateButton = nullptr;
    Ui::IButton* gizmoScaleButton = nullptr;
    EditorGizmoToolbarBinding gizmoBinding{};
};

}  // namespace Editor
}  // namespace Spark
