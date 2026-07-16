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

    void SetLayoutState(DockLayoutState value) noexcept { state = value; }
    [[nodiscard]] const DockLayoutState& GetLayoutState() const noexcept { return state; }

    void SetLeftCollapsed(bool collapsed) noexcept { state.leftCollapsed = collapsed; }
    void SetRightCollapsed(bool collapsed) noexcept { state.rightCollapsed = collapsed; }
    void ToggleLeftCollapsed() noexcept { state.leftCollapsed = !state.leftCollapsed; }
    void ToggleRightCollapsed() noexcept { state.rightCollapsed = !state.rightCollapsed; }

    void SetOnLayoutStateChanged(std::function<void(const DockLayoutState&)> fn) {
        onLayoutStateChanged = Spark::MoveTemp(fn);
    }

    [[nodiscard]] UniquePtr<DockFrameLayout> BuildFrame();

    void LoadState() noexcept;
    void SaveState() const noexcept;

    [[nodiscard]] static DockManager CreateEditorDefault();

private:
    void NotifyStateChanged(bool save);
    [[nodiscard]] UniquePtr<Widget> BuildLeftContent();
    [[nodiscard]] UniquePtr<Widget> BuildRightContent();

    Array<UniquePtr<DockPanel>> panels{};
    DockLayoutState state{};
    std::function<void(const DockLayoutState&)> onLayoutStateChanged{};
};

}  // namespace Spark::Gui
