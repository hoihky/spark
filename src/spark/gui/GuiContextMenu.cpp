#include "spark/gui/GuiContextMenu.hpp"

#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"

#include <algorithm>
#include <cmath>

namespace Spark::Gui {

namespace {

GuiContextMenu gMenu{};

}  // namespace

GuiContextMenu& GetGuiContextMenu() noexcept {
    return gMenu;
}

void GuiContextMenu::Close() noexcept {
    open = false;
    items.Clear();
    onPick = nullptr;
    hoverIndex = -1;
}

void GuiContextMenu::Open(
        const float x,
        const float y,
        Array<Utf8String> itemLabels,
        std::function<void(int index)> pick) {
    Close();
    if (itemLabels.IsEmpty()) {
        return;
    }
    open = true;
    anchorX = x;
    anchorY = y;
    items = MoveTemp(itemLabels);
    onPick = MoveTemp(pick);
    hoverIndex = -1;
    rowHeight = GetActiveGuiLayoutMetrics().ContextMenuRowHeight();
    const float w = 300.0F;
    const float h = rowHeight * static_cast<float>(items.GetSize()) + GetActiveGuiLayoutMetrics().Padding();
    panelRect = {x, y, w, h};
}

bool GuiContextMenu::HandlePointer(const GuiFrameInput& in, GuiCanvasComponent& /*canvas*/) {
    if (!open) {
        return false;
    }
    hoverIndex = -1;
    if (panelRect.Contains(in.mouseX, in.mouseY)) {
        const float localY = in.mouseY - panelRect.y - 4.0F;
        if (localY >= 0.0F) {
            const int idx = static_cast<int>(localY / rowHeight);
            if (idx >= 0 && static_cast<std::size_t>(idx) < items.GetSize()) {
                hoverIndex = idx;
            }
        }
    }
    if (in.leftPressedThisFrame) {
        if (hoverIndex >= 0 && onPick) {
            onPick(hoverIndex);
        }
        Close();
        return true;
    }
    if (in.rightPressedThisFrame && !panelRect.Contains(in.mouseX, in.mouseY)) {
        Close();
        return true;
    }
    return true;
}

void GuiContextMenu::Paint(GuiPaintContext& ctx) const {
    if (!open) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    ctx.PushOverlayLayer();
    ctx.FillRoundRectGradientVertical(
            panelRect.x,
            panelRect.y,
            panelRect.width,
            panelRect.height,
            th.controlCornerRadius,
            th.dropdownPanelTop,
            th.dropdownPanelBottom,
            th.dropdownPanelAlpha);
    ctx.StrokeRoundRect(
            panelRect.x,
            panelRect.y,
            panelRect.width,
            panelRect.height,
            th.controlCornerRadius,
            1.0F,
            th.borderRgb,
            0.75F);
    for (std::size_t i = 0; i < items.GetSize(); ++i) {
        const float ry = panelRect.y + 4.0F + static_cast<float>(i) * rowHeight;
        if (static_cast<int>(i) == hoverIndex) {
            ctx.FillRect(panelRect.x + 4.0F, ry, panelRect.width - 8.0F, rowHeight - 2.0F, th.controlHotTop, 0.55F);
        }
        ctx.DrawText(panelRect.x + 12.0F, ry + 6.0F, GetActiveGuiLayoutMetrics().FontBody(), items[i], th.labelPrimary,
                1.0F);
    }
    ctx.PopOverlayLayer();
}

}  // namespace Spark::Gui
