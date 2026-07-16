#pragma once

#include "spark/gui/Widget.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark::Gui {

class DockSidePane;
class GuiPaintContext;

/**
 * Three-region editor frame: left dock rail | center passthrough | right dock rail.
 */
class DockFrameLayout final : public Widget {
public:
    void SetLeftPane(UniquePtr<DockSidePane> pane);
    void SetCenter(UniquePtr<Widget> center);
    void SetRightPane(UniquePtr<DockSidePane> pane);

    [[nodiscard]] DockSidePane* GetLeftPane() noexcept { return leftPane; }
    [[nodiscard]] DockSidePane* GetRightPane() noexcept { return rightPane; }
    /** Center passthrough region in framebuffer pixels (updated each <c>Arrange</c>). */
    [[nodiscard]] const Rect& GetCenterBounds() const noexcept { return centerBounds; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;

private:
    DockSidePane* leftPane = nullptr;
    DockSidePane* rightPane = nullptr;
    Widget* center = nullptr;
    Rect centerBounds{};
};

}  // namespace Spark::Gui
