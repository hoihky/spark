#pragma once

#include "spark/editor/IEditorPanel.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/docking/DockManager.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark::Editor {

/**
 * Top-level editor chrome: menu bar + toolbar + OOP dock frame (collapsible left/right panels).
 */
class EditorDockShell {
public:
    void SetSidebarWidth(float widthPx) noexcept;
    [[nodiscard]] float GetSidebarWidth() const noexcept { return sidebarWidthPx_; }
    [[nodiscard]] float GetRightPanelWidth() const noexcept { return dockManager_.GetLayoutState().rightWidthPx; }

    void ToggleLeftPanel() noexcept;
    void ToggleRightPanel() noexcept;

    void SetPanels(
            UniquePtr<Gui::Widget> hierarchyRoot,
            UniquePtr<Gui::Widget> projectRoot,
            UniquePtr<Gui::Widget> inspectorRoot);

    [[nodiscard]] Gui::Widget* GetRootWidget() noexcept { return root_.Get(); }
    [[nodiscard]] UniquePtr<Gui::Widget> ReleaseRootWidget() { return MoveTemp(root_); }
    /** 3D viewport region in framebuffer pixels (center pane after layout). */
    [[nodiscard]] Gui::Rect GetWorldViewportRect() const noexcept;

private:
    void Rebuild(
            UniquePtr<Gui::Widget> hierarchyRoot,
            UniquePtr<Gui::Widget> projectRoot,
            UniquePtr<Gui::Widget> inspectorRoot);

    Gui::DockManager dockManager_{};
    Gui::DockFrameLayout* dockFrame_ = nullptr;
    float sidebarWidthPx_ = 300.0F;
    UniquePtr<Gui::Widget> root_;
};

}  // namespace Spark::Editor
