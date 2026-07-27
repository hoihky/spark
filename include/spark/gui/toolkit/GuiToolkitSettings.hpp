#pragma once

#include "spark/gui/toolkit/GuiToolkitKind.hpp"

namespace Spark::Gui {

/**
 * Process-wide preference for which UI toolkit new game/editor code should use.
 * Prefer the portable API in <c>spark/gui/api/GuiApi.hpp</c> (<c>Gui::Ui()</c>) so implementations can be swapped.
 */
class GuiToolkitSettings {
public:
    [[nodiscard]] static GuiToolkitKind GetPreferred() noexcept;
    static void SetPreferred(GuiToolkitKind kind) noexcept;

    /** When true, <c>ProcessGuiCanvasesInput</c> should run for Spark retained canvases. */
    [[nodiscard]] static bool ShouldProcessSparkGuiInput() noexcept;

private:
    static GuiToolkitKind preferred;
};

}  // namespace Spark::Gui
