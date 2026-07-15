#pragma once

#include "spark/gui/DockLayoutModel.hpp"

namespace Spark::Gui {

/** Load/save <c>DockLayoutModel</c> to <c>dock_layout.ini</c> under the build assets dir. */
[[nodiscard]] bool TryLoadDockLayout(DockLayoutModel& out) noexcept;
[[nodiscard]] bool SaveDockLayout(const DockLayoutModel& model) noexcept;

}  // namespace Spark::Gui
