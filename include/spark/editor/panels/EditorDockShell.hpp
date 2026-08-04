#pragma once

#include "spark/editor/IEditorPanel.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark::Editor {

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

    [[nodiscard]] Ui::IUiElement* GetRootElement() noexcept { return root.Get(); }
    [[nodiscard]] UniquePtr<Ui::IUiElement> ReleaseRootElement() { return MoveTemp(root); }
    /** 3D viewport region in framebuffer pixels (center pane after layout). */
    [[nodiscard]] Ui::Rect GetWorldViewportRect() const noexcept;

private:
    void Rebuild(
            UniquePtr<Ui::IUiElement> hierarchyRoot,
            UniquePtr<Ui::IUiElement> projectRoot,
            UniquePtr<Ui::IUiElement> inspectorRoot);

    Ui::IDockWorkspace* dockWorkspace = nullptr;
    float sidebarWidthPx = 300.0F;
    UniquePtr<Ui::IUiElement> root;
};

}  // namespace Spark::Editor
