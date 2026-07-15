#pragma once

namespace Spark::Gui {

/** Persisted widths and collapse flags for <c>DockManager</c> editor chrome. */
struct DockLayoutState {
    float leftWidthPx = 300.0F;
    float rightWidthPx = 320.0F;
    bool leftCollapsed = false;
    bool rightCollapsed = false;
    int leftSelectedTab = 0;
};

[[nodiscard]] bool TryLoadDockLayoutState(DockLayoutState& out) noexcept;
[[nodiscard]] bool SaveDockLayoutState(const DockLayoutState& state) noexcept;

}  // namespace Spark::Gui
