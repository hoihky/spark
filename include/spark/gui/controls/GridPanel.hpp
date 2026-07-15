#pragma once

#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/**
 * Fixed-column flow layout (left→right, then next row). Fits tile palettes, brush grids, and tool strips.
 */
class GridPanel final : public Widget {
public:
    void SetColumns(std::uint32_t c) noexcept { columns = c < 1 ? 1 : c; }
    [[nodiscard]] std::uint32_t GetColumns() const noexcept { return columns; }
    void SetHorizontalSpacing(float s) noexcept { hSpacing = s; }
    void SetVerticalSpacing(float s) noexcept { vSpacing = s; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    std::uint32_t columns = 4;
    float hSpacing = 6.0F;
    float vSpacing = 6.0F;
};

}  // namespace Spark::Gui
