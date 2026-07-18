#include "spark/gui/controls/MenuBar.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/GuiWidgetPopups.hpp"

#include <algorithm>

namespace Spark::Gui {

void MenuBar::ClosePopup() noexcept {
    openMenuIndex = -1;
}

void MenuBar::DismissPopupUnlessHit(const float mouseX, const float mouseY, const Widget* hitWidget) noexcept {
    if (openMenuIndex < 0) {
        return;
    }
    if (hitWidget != nullptr && hitWidget != this && IsDescendantOf(hitWidget, this)) {
        return;
    }
    if (popupRect.Contains(mouseX, mouseY)) {
        return;
    }
    const std::size_t n = items.GetSize();
    if (n == 0) {
        ClosePopup();
        return;
    }
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float gap = m.ControlGap();
    const float scaledBarH = m.Scaled(barHeight);
    const float totalGap = gap * static_cast<float>(n > 1 ? n - 1 : 0);
    const float btnW = std::max(m.Scaled(48.0F), (bounds.width - totalGap) / static_cast<float>(n));
    float bx = bounds.x;
    for (std::size_t i = 0; i < n; ++i) {
        const Rect br{bx, bounds.y, btnW, scaledBarH};
        if (br.Contains(mouseX, mouseY)) {
            return;
        }
        bx += btnW + gap;
    }
    ClosePopup();
}

void MenuBar::Arrange(const Rect& r) {
    bounds = r;
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float scaledBarH = m.Scaled(barHeight);
    const float menuRowH = m.DropdownRowHeight();
    const float gap = m.ControlGap();
    const std::size_t n = items.GetSize();
    if (n == 0) {
        return;
    }
    const float totalGap = gap * static_cast<float>(n > 1 ? n - 1 : 0);
    const float btnW = std::max(m.Scaled(48.0F), (r.width - totalGap) / static_cast<float>(n));
    float x = r.x;
    for (std::size_t i = 0; i < n; ++i) {
        const Rect br{x, r.y, btnW, scaledBarH};
        x += btnW + gap;
        if (openMenuIndex >= 0 && static_cast<std::size_t>(openMenuIndex) == i) {
            const std::size_t rows = items[i].dropdownEntries.GetSize();
            const float ph = static_cast<float>(rows) * menuRowH + m.Padding();
            popupRect = {br.x, br.y + scaledBarH, std::max(br.width, m.Scaled(160.0F)), ph};
        }
    }
}

void MenuBar::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    const GuiLayoutMetrics& m = ctx.GetLayoutMetrics();
    const float scaledBarH = m.Scaled(barHeight);
    const float menuRowH = m.DropdownRowHeight();
    const float menuFont = m.FontSmall();
    const float gap = m.ControlGap();
    const std::size_t n = items.GetSize();
    if (n == 0) {
        return;
    }
    const float totalGap = gap * static_cast<float>(n > 1 ? n - 1 : 0);
    const float btnW = std::max(m.Scaled(48.0F), (bounds.width - totalGap) / static_cast<float>(n));
    ctx.FillRoundRectGradientVertical(bounds.x, bounds.y, bounds.width, scaledBarH, 0.0F, th.tabHeaderTop,
            th.tabHeaderBottom, th.tabHeaderAlpha);
    float x = bounds.x;
    for (std::size_t i = 0; i < n; ++i) {
        const bool hot = ctx.IsHot(this) && openMenuIndex < 0;
        const bool open = static_cast<int>(i) == openMenuIndex;
        const Vector3 top = open ? th.controlAccentTop : (hot ? th.controlHotTop : th.controlIdleTop);
        const Vector3 bot = open ? th.controlAccentBottom : (hot ? th.controlHotBottom : th.controlIdleBottom);
        ctx.FillRoundRectGradientVertical(x, bounds.y, btnW, scaledBarH, m.Scaled(th.controlCornerRadius), top, bot,
                th.controlFillAlpha);
        const float pad = m.Scaled(10.0F);
        const float labelW = std::max(0.0F, btnW - pad * 2.0F);
        const float labelY = bounds.y + std::max(0.0F, (scaledBarH - menuFont) * 0.5F);
        const Utf8String drawn = ctx.EllipsizeUtf8(items[i].label, menuFont, labelW);
        ctx.DrawText(x + pad, labelY, menuFont, drawn, open ? th.labelOnAccent : th.labelPrimary, 1.0F, false);
        x += btnW + gap;
    }
    if (openMenuIndex >= 0 && static_cast<std::size_t>(openMenuIndex) < n) {
        ctx.PushOverlayLayer();
        ctx.FillRoundRectGradientVertical(popupRect.x, popupRect.y, popupRect.width, popupRect.height,
                m.Scaled(th.controlCornerRadius), th.dropdownPanelTop, th.dropdownPanelBottom, th.dropdownPanelAlpha);
        const auto& ent = items[static_cast<std::size_t>(openMenuIndex)].dropdownEntries;
        const float rowPadX = m.Scaled(10.0F);
        for (std::size_t r = 0; r < ent.GetSize(); ++r) {
            const float rowY = popupRect.y + m.Scaled(6.0F) + static_cast<float>(r) * menuRowH;
            const float rowW = std::max(0.0F, popupRect.width - rowPadX * 2.0F);
            const float rowTextY = rowY + std::max(0.0F, (menuRowH - menuFont) * 0.5F);
            const Utf8String drawn = ctx.EllipsizeUtf8(ent[r], menuFont, rowW);
            ctx.DrawText(popupRect.x + rowPadX, rowTextY, menuFont, drawn, th.labelPrimary, 1.0F, false);
        }
        ctx.PopOverlayLayer();
    }
}

Widget* MenuBar::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (openMenuIndex >= 0 && popupRect.Contains(x, y)) {
        return this;
    }
    if (!bounds.Contains(x, y)) {
        return nullptr;
    }
    const std::size_t n = items.GetSize();
    if (n == 0) {
        return nullptr;
    }
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float gap = m.ControlGap();
    const float scaledBarH = m.Scaled(barHeight);
    const float totalGap = gap * static_cast<float>(n > 1 ? n - 1 : 0);
    const float btnW = std::max(m.Scaled(48.0F), (bounds.width - totalGap) / static_cast<float>(n));
    float bx = bounds.x;
    for (std::size_t i = 0; i < n; ++i) {
        const Rect br{bx, bounds.y, btnW, scaledBarH};
        if (br.Contains(x, y)) {
            return this;
        }
        bx += btnW + gap;
    }
    return nullptr;
}

void MenuBar::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& /*canvas*/) {
    if (!enabled) {
        return;
    }
    if (openMenuIndex >= 0 && popupRect.Contains(in.mouseX, in.mouseY)) {
        const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
        const float menuRowH = m.DropdownRowHeight();
        const float localY = in.mouseY - popupRect.y - m.Scaled(6.0F);
        const int row = static_cast<int>(localY / menuRowH);
        if (row >= 0 && static_cast<std::size_t>(row) < items[static_cast<std::size_t>(openMenuIndex)].dropdownEntries.GetSize()) {
            if (items[static_cast<std::size_t>(openMenuIndex)].onDropdownSelect) {
                items[static_cast<std::size_t>(openMenuIndex)].onDropdownSelect(row);
            }
        }
        ClosePopup();
        return;
    }
    const std::size_t n = items.GetSize();
    if (n == 0) {
        return;
    }
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float gap = m.ControlGap();
    const float scaledBarH = m.Scaled(barHeight);
    const float totalGap = gap * static_cast<float>(n > 1 ? n - 1 : 0);
    const float btnW = std::max(m.Scaled(48.0F), (bounds.width - totalGap) / static_cast<float>(n));
    float bx = bounds.x;
    for (std::size_t i = 0; i < n; ++i) {
        const Rect br{bx, bounds.y, btnW, scaledBarH};
        if (br.Contains(in.mouseX, in.mouseY)) {
            if (!items[i].dropdownEntries.IsEmpty()) {
                openMenuIndex = static_cast<int>(i);
            } else if (items[i].onActivate) {
                items[i].onActivate();
                ClosePopup();
            }
            return;
        }
        bx += btnW + gap;
    }
    ClosePopup();
}

}  // namespace Spark::Gui
