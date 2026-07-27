#include "spark/gui/api/SparkNativeImmediateGuiFrame.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiThemeCatalog.hpp"
#include "spark/text/Font.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace Spark::Gui {

namespace {

std::uint32_t HashId(const char* id) noexcept {
    std::uint32_t hash = 2166136261U;
    if (id == nullptr) {
        return hash;
    }
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(id); *p != 0; ++p) {
        hash ^= static_cast<std::uint32_t>(*p);
        hash *= 16777619U;
    }
    return hash;
}

struct ControlRect {
    float x = 0.0F;
    float y = 0.0F;
    float w = 0.0F;
    float h = 0.0F;
};

ControlRect gLastButtonRect{};
bool gButtonClicked = false;

bool PointInRect(const float px, const float py, const ControlRect& r) noexcept {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

const GuiLayoutMetrics& Layout() noexcept {
    return GetActiveGuiLayoutMetrics();
}

float Px(const float designPixels) noexcept {
    return designPixels * Layout().uiScale;
}

float EstimateTextWidth(const char* text, const float fontPx) noexcept {
    if (text == nullptr) {
        return 0.0F;
    }
    std::size_t len = std::strlen(text);
    return static_cast<float>(len) * fontPx * 0.52F;
}

}  // namespace

void SparkNativeImmediateGuiFrame::ResetForFrame() noexcept {
    const GuiLayoutMetrics& m = Layout();
    const float inset = Px(16.0F);
    cursorX = inset;
    cursorY = inset;
    lineHeight = std::max(m.FormRowHeight(), m.FontBody() + m.ControlGap() * 2.0F);
    contentWidth = Px(520.0F);
    panelDepth = 0;
    sameLine = false;
    panelOuterX = inset;
    panelOuterY = inset;
    panelOuterW = 0.0F;
    panelOuterH = 0.0F;
    pendingPanelW = 0.0F;
    pendingPanelH = 0.0F;
    contentStartX = inset;
    lineCursorX = inset;
    gButtonClicked = false;
    if (frameContext != nullptr && frameContext->framebufferWidth > 0) {
        const float fbW = static_cast<float>(frameContext->framebufferWidth);
        contentWidth = std::min(Px(520.0F), fbW - Px(32.0F));
        panelOuterW = contentWidth + Px(28.0F);
    }
}

void SparkNativeImmediateGuiFrame::Text(const char* text) {
    if (frameContext == nullptr || frameContext->renderParams == nullptr || text == nullptr) {
        return;
    }
    GuiPaintContext ctx(*frameContext->renderParams);
    if (frameContext->uiFont != nullptr) {
        ctx.SetLayoutFont(frameContext->uiFont);
    }
    const GuiTheme& theme = ResolveGuiTheme(GetActiveGuiThemePreset());
    ctx.SetTheme(&theme);
    const GuiLayoutMetrics& m = Layout();
    const bool onNewLine = !sameLine;
    if (onNewLine) {
        cursorX = contentStartX;
    } else {
        cursorX = lineCursorX + Px(8.0F);
    }
    const float y = sameLine ? sameLineY : cursorY;
    sameLine = false;
    ctx.DrawText(cursorX, y, m.FontBody(), Utf8String(text), theme.labelPrimary, 1.0F);
    lineCursorX = cursorX + EstimateTextWidth(text, m.FontBody());
    if (onNewLine) {
        cursorY += lineHeight;
    }
}

void SparkNativeImmediateGuiFrame::TextDisabled(const char* text) {
    if (frameContext == nullptr || frameContext->renderParams == nullptr || text == nullptr) {
        return;
    }
    GuiPaintContext ctx(*frameContext->renderParams);
    if (frameContext->uiFont != nullptr) {
        ctx.SetLayoutFont(frameContext->uiFont);
    }
    const GuiTheme& theme = ResolveGuiTheme(GetActiveGuiThemePreset());
    ctx.SetTheme(&theme);
    const GuiLayoutMetrics& m = Layout();
    const bool onNewLine = !sameLine;
    if (onNewLine) {
        cursorX = contentStartX;
    } else {
        cursorX = lineCursorX + Px(8.0F);
    }
    const float y = sameLine ? sameLineY : cursorY;
    sameLine = false;
    ctx.DrawText(cursorX, y, m.FontBody(), Utf8String(text), theme.labelMuted, 0.85F);
    lineCursorX = cursorX + EstimateTextWidth(text, m.FontBody());
    if (onNewLine) {
        cursorY += lineHeight;
    }
}

void SparkNativeImmediateGuiFrame::Separator() {
    if (frameContext == nullptr || frameContext->renderParams == nullptr) {
        return;
    }
    sameLine = false;
    cursorX = contentStartX;
    lineCursorX = contentStartX;
    GuiPaintContext ctx(*frameContext->renderParams);
    const GuiTheme& theme = ResolveGuiTheme(GetActiveGuiThemePreset());
    ctx.SetTheme(&theme);
    const float y = cursorY + Px(6.0F);
    ctx.FillRect(cursorX, y, contentWidth, Px(1.0F), theme.borderRgb, 0.6F);
    cursorY += Px(14.0F);
}

bool SparkNativeImmediateGuiFrame::Button(const char* id, const char* label) {
    if (frameContext == nullptr || frameContext->renderParams == nullptr || id == nullptr) {
        return false;
    }
    const GuiLayoutMetrics& m = Layout();
    const float h = m.FormRowHeight();
    const bool inlineRow = sameLine;
    const float btnW = inlineRow ? Px(56.0F) : contentWidth;
    const float y = inlineRow ? sameLineY : cursorY;
    float x = cursorX;
    if (!inlineRow) {
        x = contentStartX;
        cursorX = contentStartX;
    }
    const ControlRect rect{x, y, btnW, h};
    gLastButtonRect = rect;
    sameLine = false;

    bool hot = false;
    bool pressed = false;
    if (frameContext->input != nullptr) {
        float mx = 0.0F;
        float my = 0.0F;
        frameContext->input->GetCursorFramebufferPixels(
                mx, my, frameContext->framebufferWidth, frameContext->framebufferHeight);
        hot = PointInRect(mx, my, rect);
        pressed = hot && frameContext->input->IsMouseButtonPressedThisFrame(0);
    }

    GuiPaintContext ctx(*frameContext->renderParams);
    if (frameContext->uiFont != nullptr) {
        ctx.SetLayoutFont(frameContext->uiFont);
    }
    const GuiTheme& theme = ResolveGuiTheme(GetActiveGuiThemePreset());
    ctx.SetTheme(&theme);
    Vector3 top = hot ? theme.controlHotTop : theme.controlIdleTop;
    Vector3 bot = hot ? theme.controlHotBottom : theme.controlIdleBottom;
    const float corner = Px(6.0F);
    ctx.FillRoundRectGradientVertical(rect.x, rect.y, rect.w, rect.h, corner, top, bot, theme.controlFillAlpha);
    ctx.StrokeRoundRect(rect.x, rect.y, rect.w, rect.h, corner, Px(1.0F), theme.borderRgb, theme.controlStrokeAlpha);
    const char* caption = label != nullptr ? label : id;
    ctx.DrawText(
            rect.x + Px(12.0F),
            rect.y + (rect.h - m.FontBody()) * 0.5F,
            m.FontBody(),
            Utf8String(caption),
            theme.labelPrimary,
            1.0F);

    if (inlineRow) {
        cursorX = x + btnW + Px(8.0F);
        lineCursorX = cursorX;
        cursorY = std::max(cursorY, y + h + Px(8.0F));
    } else {
        cursorY = y + h + Px(8.0F);
        lineCursorX = contentStartX + contentWidth;
    }
    (void)HashId(id);
    if (pressed) {
        gButtonClicked = true;
        return true;
    }
    return false;
}

bool SparkNativeImmediateGuiFrame::Checkbox(const char* id, const char* label, bool& value) {
    if (frameContext == nullptr || frameContext->renderParams == nullptr || id == nullptr) {
        return false;
    }
    sameLine = false;
    cursorX = contentStartX;
    lineCursorX = contentStartX;
    const GuiLayoutMetrics& m = Layout();
    const float box = Px(20.0F);
    const float h = m.FormRowHeight();
    const float y = cursorY;
    ControlRect boxRect{cursorX, y + Px(3.0F), box, box};

    bool hot = false;
    bool toggled = false;
    if (frameContext->input != nullptr) {
        float mx = 0.0F;
        float my = 0.0F;
        frameContext->input->GetCursorFramebufferPixels(
                mx, my, frameContext->framebufferWidth, frameContext->framebufferHeight);
        hot = PointInRect(mx, my, boxRect);
        if (hot && frameContext->input->IsMouseButtonPressedThisFrame(0)) {
            value = !value;
            toggled = true;
        }
    }

    GuiPaintContext ctx(*frameContext->renderParams);
    const GuiTheme& theme = ResolveGuiTheme(GetActiveGuiThemePreset());
    ctx.SetTheme(&theme);
    ctx.FillRoundRectSolid(boxRect.x, boxRect.y, boxRect.w, boxRect.h, Px(4.0F), theme.controlIdleTop, 1.0F);
    if (value) {
        ctx.FillRoundRectSolid(
                boxRect.x + Px(4.0F),
                boxRect.y + Px(4.0F),
                boxRect.w - Px(8.0F),
                boxRect.h - Px(8.0F),
                Px(2.0F),
                theme.controlAccentTop,
                1.0F);
    }
    if (label != nullptr) {
        ctx.DrawText(boxRect.x + box + Px(8.0F), y + Px(2.0F), m.FontBody(), Utf8String(label), theme.labelPrimary, 1.0F);
    }
    cursorY += h + Px(6.0F);
    (void)HashId(id);
    (void)hot;
    return toggled;
}

bool SparkNativeImmediateGuiFrame::SliderFloat(
        const char* id,
        const char* label,
        float& value,
        const float minValue,
        const float maxValue) {
    if (frameContext == nullptr || frameContext->renderParams == nullptr || id == nullptr) {
        return false;
    }
    sameLine = false;
    cursorX = contentStartX;
    if (label != nullptr) {
        Text(label);
    }
    const float trackH = Px(12.0F);
    const float y = cursorY + Px(4.0F);
    const ControlRect track{contentStartX, y, contentWidth, trackH};
    bool changed = false;
    if (frameContext->input != nullptr && frameContext->input->IsMouseButtonDown(0)) {
        float mx = 0.0F;
        float my = 0.0F;
        frameContext->input->GetCursorFramebufferPixels(
                mx, my, frameContext->framebufferWidth, frameContext->framebufferHeight);
        if (PointInRect(mx, my, track)) {
            const float t = std::clamp((mx - track.x) / std::max(track.w, 1.0F), 0.0F, 1.0F);
            value = minValue + t * (maxValue - minValue);
            changed = true;
        }
    }
    GuiPaintContext ctx(*frameContext->renderParams);
    const GuiTheme& theme = ResolveGuiTheme(GetActiveGuiThemePreset());
    ctx.SetTheme(&theme);
    ctx.FillRoundRectSolid(track.x, track.y, track.w, track.h, Px(4.0F), theme.controlIdleBottom, 1.0F);
    const float t = (value - minValue) / std::max(maxValue - minValue, 0.0001F);
    const float knobW = Px(14.0F);
    const float knobX = track.x + track.w * std::clamp(t, 0.0F, 1.0F) - knobW * 0.5F;
    ctx.FillRoundRectSolid(knobX, track.y - Px(4.0F), knobW, track.h + Px(8.0F), Px(6.0F), theme.controlAccentTop, 1.0F);
    cursorY = track.y + track.h + Px(20.0F);
    (void)HashId(id);
    return changed;
}

bool SparkNativeImmediateGuiFrame::DragFloat3(const char* id, const char* label, float values[3], const float speed) {
    if (values == nullptr) {
        return false;
    }
    bool changed = false;
    for (int i = 0; i < 3; ++i) {
        char subId[64];
        snprintf(subId, sizeof(subId), "%s_%d", id != nullptr ? id : "v", i);
        char subLabel[32];
        snprintf(subLabel, sizeof(subLabel), "%s[%d]", label != nullptr ? label : "Value", i);
        float v = values[i];
        if (SliderFloat(subId, subLabel, v, v - 10.0F * speed, v + 10.0F * speed)) {
            values[i] = v;
            changed = true;
        }
    }
    return changed;
}

bool SparkNativeImmediateGuiFrame::Combo(
        const char* id,
        const char* label,
        int& currentItem,
        const char* const items[],
        const int itemCount) {
    if (items == nullptr || itemCount <= 0 || currentItem < 0 || currentItem >= itemCount) {
        return false;
    }
    if (label != nullptr) {
        Text(label);
    }
    const char* preview = items[currentItem];
  char buttonLabel[128];
    snprintf(buttonLabel, sizeof(buttonLabel), "%s ##combo", preview != nullptr ? preview : "");
    bool changed = false;
    if (Button(id != nullptr ? id : "combo", buttonLabel)) {
        currentItem = (currentItem + 1) % itemCount;
        changed = true;
    }
    return changed;
}

bool SparkNativeImmediateGuiFrame::BeginPanel(const char* id, const char* title, bool* open) {
    if (open != nullptr && !*open) {
        return false;
    }
    (void)id;
    if (panelDepth == 0 && frameContext != nullptr && frameContext->renderParams != nullptr) {
        const GuiLayoutMetrics& m = Layout();
        const float pad = Px(14.0F);
        if (panelOuterW <= 0.0F) {
            panelOuterX = cursorX - pad;
            panelOuterY = cursorY;
            panelOuterW = contentWidth + pad * 2.0F;
        }
        const float fbH = static_cast<float>((std::max)(1, frameContext->framebufferHeight));
        float panelH = panelOuterH;
        if (panelH <= 0.0F) {
            panelH = std::max(Px(120.0F), fbH - panelOuterY - Px(16.0F));
        }

        GuiPaintContext ctx(*frameContext->renderParams);
        const GuiTheme& theme = ResolveGuiTheme(GetActiveGuiThemePreset());
        ctx.SetTheme(&theme);
        const float corner = Px(10.0F);
        ctx.FillRoundRectGradientVertical(
                panelOuterX,
                panelOuterY,
                panelOuterW,
                panelH,
                corner,
                theme.panelElevatedTop,
                theme.panelElevatedBottom,
                theme.panelElevatedAlpha);
        ctx.StrokeRoundRect(
                panelOuterX,
                panelOuterY,
                panelOuterW,
                panelH,
                corner,
                Px(1.0F),
                theme.borderRgb,
                theme.controlStrokeAlpha);

        cursorX = panelOuterX + pad;
        cursorY = panelOuterY + pad;
        contentStartX = cursorX;
        lineCursorX = cursorX;
        contentWidth = std::max(Px(300.0F), panelOuterW - pad * 2.0F);
        if (title != nullptr) {
            if (frameContext->uiFont != nullptr) {
                ctx.SetLayoutFont(frameContext->uiFont);
            }
            ctx.DrawText(cursorX, cursorY, m.FontLabel(), Utf8String(title), theme.labelPrimary, 1.0F);
            cursorY += lineHeight + Px(6.0F);
        }
    } else if (title != nullptr) {
        Text(title);
        cursorX += Px(8.0F);
        contentWidth = std::max(Px(120.0F), contentWidth - Px(16.0F));
    }
    ++panelDepth;
    return true;
}

void SparkNativeImmediateGuiFrame::EndPanel() {
    if (panelDepth > 0) {
        --panelDepth;
        if (panelDepth == 0) {
            panelOuterW = 0.0F;
            panelOuterH = 0.0F;
        } else {
            cursorX = std::max(Px(16.0F), cursorX - Px(8.0F));
            contentWidth += Px(16.0F);
        }
    }
}

void SparkNativeImmediateGuiFrame::SameLine(const float offsetFromStartX, const float /*spacing*/) {
    sameLine = true;
    if (offsetFromStartX > 0.0F) {
        cursorX = contentStartX + offsetFromStartX;
    } else {
        cursorX = lineCursorX + Px(8.0F);
    }
    sameLineY = cursorY - lineHeight;
}

bool SparkNativeImmediateGuiFrame::Selectable(const char* id, const char* label, const bool selected) {
    if (frameContext == nullptr || frameContext->renderParams == nullptr || id == nullptr) {
        return false;
    }
    sameLine = false;
    cursorX = contentStartX;
    lineCursorX = contentStartX;
    const GuiLayoutMetrics& m = Layout();
    const float h = m.ListRowHeight();
    const float y = cursorY;
    const ControlRect rect{cursorX, y, contentWidth, h};

    bool hot = false;
    bool clicked = false;
    if (frameContext->input != nullptr) {
        float mx = 0.0F;
        float my = 0.0F;
        frameContext->input->GetCursorFramebufferPixels(
                mx, my, frameContext->framebufferWidth, frameContext->framebufferHeight);
        hot = PointInRect(mx, my, rect);
        clicked = hot && frameContext->input->IsMouseButtonPressedThisFrame(0);
    }

    GuiPaintContext ctx(*frameContext->renderParams);
    if (frameContext->uiFont != nullptr) {
        ctx.SetLayoutFont(frameContext->uiFont);
    }
    const GuiTheme& theme = ResolveGuiTheme(GetActiveGuiThemePreset());
    ctx.SetTheme(&theme);
    if (selected || hot) {
        ctx.FillRoundRectSolid(rect.x, rect.y, rect.w, rect.h, Px(4.0F), theme.controlAccentTop, selected ? 0.55F : 0.35F);
    }
    const char* caption = label != nullptr ? label : id;
    ctx.DrawText(
            rect.x + Px(10.0F),
            rect.y + (h - m.FontBody()) * 0.5F,
            m.FontBody(),
            Utf8String(caption),
            theme.labelPrimary,
            1.0F);
    cursorY += h + Px(4.0F);
    lineCursorX = contentStartX + contentWidth;
    (void)HashId(id);
    return clicked;
}

bool SparkNativeImmediateGuiFrame::MenuItem(const char* label, bool* checked) {
    bool local = checked != nullptr ? *checked : false;
    const bool clicked = Selectable(label != nullptr ? label : "menu", label, local);
    if (clicked && checked != nullptr) {
        *checked = !*checked;
    }
    return clicked;
}

void SparkNativeImmediateGuiFrame::SetNextPanelSize(const float width, const float height) {
    pendingPanelW = width;
    pendingPanelH = height;
}

void SparkNativeImmediateGuiFrame::SetCursorPos(const float x, const float y) {
    const float pad = Px(14.0F);
    panelOuterX = x;
    panelOuterY = y;
    cursorX = x + pad;
    cursorY = y + pad;
    contentStartX = cursorX;
    lineCursorX = cursorX;
    sameLine = false;
    if (frameContext != nullptr && frameContext->framebufferWidth > 0) {
        const float fbW = static_cast<float>(frameContext->framebufferWidth);
        if (pendingPanelW > 0.0F) {
            panelOuterW = pendingPanelW;
            pendingPanelW = 0.0F;
        } else {
            const float outerW = std::clamp(fbW - x - Px(12.0F), Px(460.0F), Px(720.0F));
            panelOuterW = outerW;
        }
        if (pendingPanelH > 0.0F) {
            panelOuterH = pendingPanelH;
            pendingPanelH = 0.0F;
        }
        contentWidth = std::max(Px(300.0F), panelOuterW - pad * 2.0F);
    }
}

}  // namespace Spark::Gui
