#include "spark/ui/runtime/UiContextMenu.hpp"

#include "spark/config.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/core/UiPaintContext.hpp"
#include "spark/ui/core/UiTheme.hpp"

#include <algorithm>

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace Spark::Ui {

namespace {

UiContextMenu gMenu{};

#if SPARK_ENABLE_IMGUI
/** <c>Open()</c> stores framebuffer pixels; ImGui windows use display (logical) coordinates. */
void FramebufferPixelsToImGuiDisplay(const float fbX, const float fbY, float& outX, float& outY) noexcept {
    const ImGuiIO& io = ImGui::GetIO();
    const float scaleX = io.DisplayFramebufferScale.x > 1.0e-4F ? io.DisplayFramebufferScale.x : 1.0F;
    const float scaleY = io.DisplayFramebufferScale.y > 1.0e-4F ? io.DisplayFramebufferScale.y : 1.0F;
    outX = fbX / scaleX;
    outY = fbY / scaleY;
}
#endif

}  // namespace

UiContextMenu& GetUiContextMenu() noexcept {
    return gMenu;
}

void UiContextMenu::Close() noexcept {
    open = false;
    items.Clear();
    onPick = nullptr;
    hoverIndex = -1;
    imguiPopupRequested = false;
}

void UiContextMenu::Open(
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
    rowHeight = GetActiveUiLayoutMetrics().FormRowHeight();
    const float w = 300.0F;
    const float h = rowHeight * static_cast<float>(items.GetSize()) + GetActiveUiLayoutMetrics().Padding();
    float px = x;
    float py = y;
    if (viewportWidth > 0.0F) {
        px = std::clamp(px, 0.0F, std::max(0.0F, viewportWidth - w));
    }
    if (viewportHeight > 0.0F) {
        py = std::clamp(py, 0.0F, std::max(0.0F, viewportHeight - h));
    }
    anchorX = px;
    anchorY = py;
    panelRect = {px, py, w, h};
    imguiPopupRequested = true;
}

void UiContextMenu::SetViewportBounds(const float width, const float height) noexcept {
    viewportWidth = width;
    viewportHeight = height;
    if (!open) {
        return;
    }
    const float w = panelRect.width;
    const float h = panelRect.height;
    if (viewportWidth > 0.0F) {
        panelRect.x = std::clamp(panelRect.x, 0.0F, std::max(0.0F, viewportWidth - w));
        anchorX = panelRect.x;
    }
    if (viewportHeight > 0.0F) {
        panelRect.y = std::clamp(panelRect.y, 0.0F, std::max(0.0F, viewportHeight - h));
        anchorY = panelRect.y;
    }
}

bool UiContextMenu::HandlePointer(const UiFrameInput& in) {
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

void UiContextMenu::Paint(UiPaintContext& ctx) const {
    if (!open) {
        return;
    }
    const UiTheme& th = ctx.GetTheme();
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
        const float fontPx = GetActiveUiLayoutMetrics().FontBody();
        const float textPad = 12.0F;
        const float maxTextW = panelRect.width - textPad * 2.0F;
        Utf8String line = ctx.EllipsizeUtf8(items[i], fontPx, maxTextW);
        ctx.DrawText(
                panelRect.x + textPad,
                ry + (rowHeight - fontPx) * 0.5F,
                fontPx,
                line,
                th.labelPrimary,
                1.0F);
    }
    ctx.PopOverlayLayer();
}

void UiContextMenu::PaintImGui() {
#if SPARK_ENABLE_IMGUI
    if (!open) {
        return;
    }
    if (imguiPopupRequested) {
        ImGui::OpenPopup("SparkUiContextMenu");
        imguiPopupRequested = false;
    }
    float displayX = anchorX;
    float displayY = anchorY;
    FramebufferPixelsToImGuiDisplay(anchorX, anchorY, displayX, displayY);
    ImGui::SetNextWindowPos(ImVec2(displayX, displayY), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(panelRect.width, panelRect.height), ImGuiCond_Appearing);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::BeginPopup("SparkUiContextMenu", flags)) {
        for (std::size_t i = 0; i < items.GetSize(); ++i) {
            if (ImGui::Selectable(items[i].CStr(), hoverIndex == static_cast<int>(i))) {
                if (onPick) {
                    onPick(static_cast<int>(i));
                }
                Close();
                ImGui::EndPopup();
                return;
            }
        }
        ImGui::EndPopup();
        return;
    }
    if (!ImGui::IsPopupOpen("SparkUiContextMenu", ImGuiPopupFlags_AnyPopupId)) {
        Close();
    }
#endif
}

}  // namespace Spark::Ui
