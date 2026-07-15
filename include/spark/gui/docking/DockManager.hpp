#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/docking/DockLayoutState.hpp"
#include "spark/gui/docking/DockPanel.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <functional>

namespace Spark::Gui {

class DockFrameLayout;

/**
 * Object-oriented docking host: registers <c>DockPanel</c> instances, builds a
 * <c>DockFrameLayout</c> with collapsible left/right rails, and persists layout state.
 */
class DockManager {
public:
    void RegisterPanel(UniquePtr<DockPanel> panel);
    [[nodiscard]] DockPanel* FindPanel(const Utf8String& id) noexcept;

    void SetLayoutState(DockLayoutState state) noexcept { state_ = state; }
    [[nodiscard]] const DockLayoutState& GetLayoutState() const noexcept { return state_; }

    void SetLeftCollapsed(bool collapsed) noexcept { state_.leftCollapsed = collapsed; }
    void SetRightCollapsed(bool collapsed) noexcept { state_.rightCollapsed = collapsed; }
    void ToggleLeftCollapsed() noexcept { state_.leftCollapsed = !state_.leftCollapsed; }
    void ToggleRightCollapsed() noexcept { state_.rightCollapsed = !state_.rightCollapsed; }

    void SetOnLayoutStateChanged(std::function<void(const DockLayoutState&)> fn) {
        onLayoutStateChanged_ = Spark::MoveTemp(fn);
    }

    [[nodiscard]] UniquePtr<DockFrameLayout> BuildFrame();

    void LoadState() noexcept;
    void SaveState() const noexcept;

    [[nodiscard]] static DockManager CreateEditorDefault();

private:
    void NotifyStateChanged(bool save);
    [[nodiscard]] UniquePtr<Widget> BuildLeftContent();
    [[nodiscard]] UniquePtr<Widget> BuildRightContent();

    Array<UniquePtr<DockPanel>> panels_{};
    DockLayoutState state_{};
    std::function<void(const DockLayoutState&)> onLayoutStateChanged_{};
};

}  // namespace Spark::Gui
