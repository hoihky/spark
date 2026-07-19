#include "spark/core/Utility.hpp"
#include "spark/gui/GuiPaintContext.hpp"

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/text/Font.hpp"

#include <algorithm>
#include <cmath>

namespace Spark::Gui {

namespace {

[[nodiscard]] Array<ScreenRectDraw>& UiRectBucket(
        SceneRenderParams* params, const int lateDepth, const int overlayDepth) noexcept {
    if (lateDepth > 0) {
        return params->screenLateRects;
    }
    if (overlayDepth > 0) {
        return params->screenOverlayRects;
    }
    return params->screenRects;
}

[[nodiscard]] Array<ScreenTextDraw>& UiTextBucket(
        SceneRenderParams* params, const int lateDepth, const int overlayDepth) noexcept {
    if (lateDepth > 0) {
        return params->screenLateTexts;
    }
    if (overlayDepth > 0) {
        return params->screenOverlayTexts;
    }
    return params->screenTexts;
}

[[nodiscard]] Array<ScreenSpriteDraw>& UiSpriteBucket(
        SceneRenderParams* params, const int lateDepth, const int overlayDepth) noexcept {
    if (lateDepth > 0) {
        return params->screenLateSprites;
    }
    if (overlayDepth > 0) {
        return params->screenOverlaySprites;
    }
    return params->screenSprites;
}

/** Matches <c>VulkanScreenUiPass::PrepareUiTextureUpload</c> — every atlas layer is padded to this size. */
void ComputeUiAtlasLayerExtent(
        const SceneRenderParams* params,
        std::uint32_t& outMaxW,
        std::uint32_t& outMaxH) noexcept {
    outMaxW = 1U;
    outMaxH = 1U;
    if (params == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < params->uiTextures.GetSize(); ++i) {
        const Texture2D* tex = params->uiTextures[i].Get();
        if (tex == nullptr) {
            continue;
        }
        outMaxW = std::max(outMaxW, tex->GetWidth());
        outMaxH = std::max(outMaxH, tex->GetHeight());
    }
}

/** UVs in slices are normalized to the source image; the GPU atlas layer may be larger (zero-padded). */
[[nodiscard]] Vector4 ScaleUvToUiAtlasLayer(
        const Vector4& uv,
        const Texture2D& tex,
        const std::uint32_t atlasW,
        const std::uint32_t atlasH) noexcept {
    const float uScale = static_cast<float>(tex.GetWidth()) / static_cast<float>(std::max(1U, atlasW));
    const float vScale = static_cast<float>(tex.GetHeight()) / static_cast<float>(std::max(1U, atlasH));
    return {uv.x * uScale, uv.y * vScale, uv.z * uScale, uv.w * vScale};
}

[[nodiscard]] float MeasureWidth(const Font* font, const Utf8String& s, float sizePixels, float charFallbackMul) {
    const float charFallback = sizePixels * charFallbackMul;
    if (font != nullptr) {
        return font->MeasureUtf8Width(s, sizePixels);
    }
    return static_cast<float>(s.ByteLength()) * charFallback;
}

void AppendCatSpace(Utf8String& line, const Utf8String& word) {
    if (!line.IsEmpty()) {
        line.AppendUtf8(" ");
    }
    line.AppendUtf8(word);
}

/**
 * Greedy per-codepoint split for a single token wider than maxWidth. Emits full lines into outLines;
 * returns a remainder that still fits on the current logical line (caller merges into `line`).
 */
Utf8String SplitLongWordIntoLines(
        const Font* font,
        const Utf8String& word,
        float sizePixels,
        float maxWidth,
        Array<Utf8String>& outLines) {
    Utf8String acc;
    std::uint32_t cp = 0;
    constexpr float kCharFb = 0.52F;
    for (auto it = word.Iterator(); it.NextCodepoint(cp);) {
        Utf8String trial = acc;
        trial.AppendCodepoint(cp);
        const float w = MeasureWidth(font, trial, sizePixels, kCharFb);
        if (w <= maxWidth || acc.IsEmpty()) {
            acc = Spark::MoveTemp(trial);
            continue;
        }
        if (!acc.IsEmpty()) {
            outLines.PushBack(Spark::MoveTemp(acc));
            acc.Clear();
        }
        Utf8String one;
        one.AppendCodepoint(cp);
        acc = Spark::MoveTemp(one);
    }
    return acc;
}

void BuildWrappedTextLines(
        const Font* font,
        const Utf8String& text,
        float sizePixels,
        float maxWidth,
        Array<Utf8String>& outLines) {
    outLines.Clear();
    if (text.IsEmpty() || maxWidth <= 2.0F) {
        return;
    }
    static const Utf8String kNewlineMarker("\n");
    constexpr float kCharFb = 0.52F;

    Array<Utf8String> words;
    {
        Utf8String cur;
        std::uint32_t cp = 0;
        for (auto it = text.Iterator(); it.NextCodepoint(cp);) {
            if (cp == ' ' || cp == '\t') {
                if (!cur.IsEmpty()) {
                    words.PushBack(Spark::MoveTemp(cur));
                    cur.Clear();
                }
                continue;
            }
            if (cp == '\n') {
                if (!cur.IsEmpty()) {
                    words.PushBack(Spark::MoveTemp(cur));
                    cur.Clear();
                }
                words.PushBack(kNewlineMarker);
                continue;
            }
            cur.AppendCodepoint(cp);
        }
        if (!cur.IsEmpty()) {
            words.PushBack(Spark::MoveTemp(cur));
        }
    }

    Utf8String line;
    for (std::size_t wi = 0; wi < words.GetSize(); ++wi) {
        const Utf8String& w = words[wi];
        if (w == kNewlineMarker) {
            if (!line.IsEmpty()) {
                outLines.PushBack(Spark::MoveTemp(line));
                line.Clear();
            }
            continue;
        }
        Utf8String candidate = line;
        AppendCatSpace(candidate, w);
        const float cw = MeasureWidth(font, candidate, sizePixels, kCharFb);
        if (cw <= maxWidth) {
            line = Spark::MoveTemp(candidate);
            continue;
        }
        if (!line.IsEmpty()) {
            outLines.PushBack(Spark::MoveTemp(line));
            line.Clear();
        }
        const float ww = MeasureWidth(font, w, sizePixels, kCharFb);
        if (ww <= maxWidth) {
            line = w;
        } else {
            Utf8String rem = SplitLongWordIntoLines(font, w, sizePixels, maxWidth, outLines);
            if (!rem.IsEmpty()) {
                line = Spark::MoveTemp(rem);
            }
        }
    }
    if (!line.IsEmpty()) {
        outLines.PushBack(Spark::MoveTemp(line));
    }
}

[[nodiscard]] bool IntersectClipRects(const Rect& a, const Rect& b, Rect& out) noexcept {
    const float x1 = std::max(a.x, b.x);
    const float y1 = std::max(a.y, b.y);
    const float x2 = std::min(a.x + a.width, b.x + b.width);
    const float y2 = std::min(a.y + a.height, b.y + b.height);
    if (x2 <= x1 || y2 <= y1) {
        out = {0.0F, 0.0F, 0.0F, 0.0F};
        return false;
    }
    out = {x1, y1, x2 - x1, y2 - y1};
    return true;
}

[[nodiscard]] float ClampGuiCornerRadius(const float r, const float w, const float h) noexcept {
    if (w <= 1.0F || h <= 1.0F) {
        return 0.0F;
    }
    const float maxR = 0.5F * std::min(w, h) - 0.25F;
    return std::max(0.0F, std::min(std::max(r, 0.0F), std::max(0.25F, maxR)));
}

}  // namespace

const GuiTheme& GuiPaintContext::GetTheme() const noexcept {
    static const GuiTheme kFallback = GuiTheme::ClassicMint();
    return themePtr != nullptr ? *themePtr : kFallback;
}

const GuiLayoutMetrics& GuiPaintContext::GetLayoutMetrics() const noexcept {
    static const GuiLayoutMetrics kFallback = GuiLayoutMetrics::Default();
    return metricsPtr != nullptr ? *metricsPtr : kFallback;
}

float GuiPaintContext::MeasureUtf8Width(const Utf8String& text, const float sizePixels) const noexcept {
    return MeasureWidth(layoutFont, text, sizePixels, 0.52F);
}

float GuiPaintContext::GetLineSpacingPixels(const float sizePixels) const noexcept {
    return layoutFont != nullptr ? layoutFont->GetLineSpacingPixels(sizePixels) : sizePixels * 1.2F;
}

void GuiPaintContext::BuildWrappedLines(
        const Utf8String& text,
        const float sizePixels,
        const float maxWidth,
        Array<Utf8String>& outLines) const {
    BuildWrappedTextLines(layoutFont, text, sizePixels, maxWidth, outLines);
}

Utf8String GuiPaintContext::EllipsizeUtf8(
        const Utf8String& text,
        const float sizePixels,
        const float maxWidth) const {
    if (text.IsEmpty() || maxWidth <= 2.0F) {
        return text;
    }
    constexpr float kCharFb = 0.52F;
    static const Utf8String kEllipsis = Utf8String("...");
    const float fullW = MeasureWidth(layoutFont, text, sizePixels, kCharFb);
    if (fullW <= maxWidth) {
        return text;
    }
    const float ellipsisW = MeasureWidth(layoutFont, kEllipsis, sizePixels, kCharFb);
    const float budget = maxWidth - ellipsisW;
    if (budget <= 1.0F) {
        return kEllipsis;
    }
    Utf8String result;
    std::uint32_t cp = 0;
    for (auto it = text.Iterator(); it.NextCodepoint(cp);) {
        Utf8String trial = result;
        trial.AppendCodepoint(cp);
        if (MeasureWidth(layoutFont, trial, sizePixels, kCharFb) > budget) {
            break;
        }
        result = Spark::MoveTemp(trial);
    }
    if (result.IsEmpty()) {
        return kEllipsis;
    }
    result.AppendUtf8("...");
    return result;
}

void GuiPaintContext::DrawTextInRect(
        const Rect& rect,
        const float sizePixels,
        const Utf8String& text,
        const Vector3& rgb,
        const float alpha,
        const bool bold,
        const TextLayout layout) {
    if (params == nullptr || text.IsEmpty() || rect.width <= 0.0F) {
        return;
    }

    const float layoutHeight = rect.height > 0.0F ? rect.height : sizePixels;
    const float lineStep = GetLineSpacingPixels(sizePixels);
    const float maxW = rect.width;
    const bool clipHoriz = layout.overflow == TextOverflow::Clip;
    const bool clipVert = layout.wrap == TextWrap::WordWrap && layout.overflow != TextOverflow::Visible;

    const std::size_t clipDepthBefore = clipStack.GetSize();
    if (clipHoriz || clipVert) {
        PushClipRect(rect.x, rect.y, rect.width, rect.height);
    }
    const bool pushedClip = clipStack.GetSize() > clipDepthBefore;

    if (layout.wrap == TextWrap::WordWrap) {
        Array<Utf8String> lines;
        BuildWrappedTextLines(layoutFont, text, sizePixels, maxW, lines);

        int lineLimit = layout.maxLines;
        if (lineLimit <= 0 && layoutHeight > 0.0F) {
            lineLimit = std::max(1, static_cast<int>(layoutHeight / lineStep));
        }
        if (lineLimit > 0 && static_cast<int>(lines.GetSize()) > lineLimit) {
            lines.Resize(static_cast<std::size_t>(lineLimit));
            if (layout.overflow == TextOverflow::Ellipsis && !lines.IsEmpty()) {
                lines[lines.GetSize() - 1U] = EllipsizeUtf8(lines.GetLast(), sizePixels, maxW);
            }
        }

        float cy = rect.y;
        const float bottom = rect.y + layoutHeight;
        for (std::size_t i = 0; i < lines.GetSize(); ++i) {
            if (layout.overflow != TextOverflow::Visible && cy + lineStep > bottom + 0.5F) {
                break;
            }
            DrawText(rect.x, cy, sizePixels, lines[i], rgb, alpha, bold);
            cy += lineStep;
        }
    } else {
        Utf8String drawText = text;
        if (layout.overflow == TextOverflow::Ellipsis) {
            drawText = EllipsizeUtf8(text, sizePixels, maxW);
        }
        const float textY = rect.y + std::max(0.0F, (layoutHeight - sizePixels) * 0.5F);
        DrawText(rect.x, textY, sizePixels, drawText, rgb, alpha, bold);
    }

    if (pushedClip) {
        PopClipRect();
    }
}

bool GuiPaintContext::CurrentClipAllowsDraw() const noexcept {
    if (clipStack.IsEmpty()) {
        return true;
    }
    const Rect& c = clipStack.GetLast();
    return c.width > 0.0F && c.height > 0.0F;
}

bool GuiPaintContext::ClipAllowsImmediateDraw() const noexcept {
    /** Popups on overlay/late layers must paint even when a parent scroll left a zero-area clip on the stack. */
    if (overlayDepth > 0 || lateDepth > 0) {
        return true;
    }
    return CurrentClipAllowsDraw();
}

void GuiPaintContext::AttachClipTo(ScreenRectDraw& d) const noexcept {
    if (clipStack.IsEmpty()) {
        d.clipEnabled = false;
        return;
    }
    const Rect& c = clipStack.GetLast();
    d.clipEnabled = true;
    d.clipX = c.x;
    d.clipY = c.y;
    d.clipW = c.width;
    d.clipH = c.height;
}

void GuiPaintContext::AttachClipTo(ScreenSpriteDraw& d) const noexcept {
    if (clipStack.IsEmpty()) {
        d.clipEnabled = false;
        return;
    }
    const Rect& c = clipStack.GetLast();
    d.clipEnabled = true;
    d.clipX = c.x;
    d.clipY = c.y;
    d.clipW = c.width;
    d.clipH = c.height;
}

void GuiPaintContext::AttachClipTo(ScreenTextDraw& d) const noexcept {
    if (clipStack.IsEmpty()) {
        d.clipEnabled = false;
        return;
    }
    const Rect& c = clipStack.GetLast();
    d.clipEnabled = true;
    d.clipX = c.x;
    d.clipY = c.y;
    d.clipW = c.width;
    d.clipH = c.height;
}

void GuiPaintContext::PushClipRect(const float x, const float y, const float width, const float height) {
    if (params == nullptr || width <= 0.0F || height <= 0.0F) {
        return;
    }
    const Rect next{x, y, width, height};
    if (clipStack.IsEmpty()) {
        clipStack.PushBack(next);
        return;
    }
    Rect merged;
    if (!IntersectClipRects(clipStack.GetLast(), next, merged)) {
        clipStack.PushBack(Rect{0.0F, 0.0F, 0.0F, 0.0F});
        return;
    }
    clipStack.PushBack(merged);
}

void GuiPaintContext::PopClipRect() noexcept {
    if (!clipStack.IsEmpty()) {
        clipStack.PopBack();
    }
}

void GuiPaintContext::PopOverlayLayer() noexcept {
    if (overlayDepth > 0) {
        --overlayDepth;
    }
}

void GuiPaintContext::PopLateLayer() noexcept {
    if (lateDepth > 0) {
        --lateDepth;
    }
}

void GuiPaintContext::FillRoundRectSolid(
        float x, float y, float w, float h, float cornerRadius, const Vector3& rgb, float alpha) {
    FillRoundRectGradientVertical(x, y, w, h, cornerRadius, rgb, rgb, alpha);
}

void GuiPaintContext::FillRoundRectGradientVertical(
        float x,
        float y,
        float w,
        float h,
        float cornerRadius,
        const Vector3& rgbTop,
        const Vector3& rgbBottom,
        float alpha) {
    if (params == nullptr || w <= 0.0F || h <= 0.0F || !ClipAllowsImmediateDraw()) {
        return;
    }
    const float rc = ClampGuiCornerRadius(cornerRadius, w, h);
    if (rc <= 0.5F) {
        FillRectGradientVertical(x, y, w, h, rgbTop, rgbBottom, alpha);
        return;
    }
    const int rows = static_cast<int>(std::ceil(static_cast<double>(h)));
    const float denom = std::max(h, 1.0F);
    for (int i = 0; i < rows; ++i) {
        const float py = y + static_cast<float>(i) + 0.5F;
        const float t = (static_cast<float>(i) + 0.5F) / denom;
        const Vector3 rowRgb = Vector3::Lerp(rgbTop, rgbBottom, t);
        float xl = x;
        float xr = x + w;
        if (py < y + rc) {
            const float dy = (y + rc) - py;
            const float ins = rc * rc - dy * dy;
            if (ins > 0.0F) {
                const float hs = std::sqrt(ins);
                xl = std::max(xl, x + rc - hs);
                xr = std::min(xr, x + w - rc + hs);
            }
        }
        if (py > y + h - rc) {
            const float dy = py - (y + h - rc);
            const float ins = rc * rc - dy * dy;
            if (ins > 0.0F) {
                const float hs = std::sqrt(ins);
                xl = std::max(xl, x + rc - hs);
                xr = std::min(xr, x + w - rc + hs);
            }
        }
        const float rw = xr - xl;
        if (rw > 0.25F) {
            FillRect(xl, y + static_cast<float>(i), rw, 1.0F, rowRgb, alpha);
        }
    }
}

void GuiPaintContext::StrokeRoundRect(
        float x,
        float y,
        float w,
        float h,
        float /*cornerRadius*/,
        float thickness,
        const Vector3& rgb,
        float alpha) {
    StrokeRect(x, y, w, h, thickness, rgb, alpha);
}

void GuiPaintContext::FillRect(
        float x,
        float y,
        float w,
        float h,
        const Vector3& rgb,
        float alpha,
        const SceneBlendMode blend) {
    if (params == nullptr || w <= 0.0F || h <= 0.0F || !ClipAllowsImmediateDraw()) {
        return;
    }
    ScreenRectDraw d{};
    d.x = x;
    d.y = y;
    d.width = w;
    d.height = h;
    d.color = rgb;
    d.colorB = rgb;
    d.alpha = alpha;
    d.gradient = ScreenRectGradient::None;
    d.blendMode = blend;
    AttachClipTo(d);
    d.paintOrder = params->NextUiPaintOrder();
    UiRectBucket(params, lateDepth, overlayDepth).PushBack(d);
}

void GuiPaintContext::FillRectGradientVertical(
        float x,
        float y,
        float w,
        float h,
        const Vector3& rgbTop,
        const Vector3& rgbBottom,
        float alpha,
        const SceneBlendMode blend) {
    if (params == nullptr || w <= 0.0F || h <= 0.0F || !ClipAllowsImmediateDraw()) {
        return;
    }
    ScreenRectDraw d{};
    d.x = x;
    d.y = y;
    d.width = w;
    d.height = h;
    d.color = rgbTop;
    d.colorB = rgbBottom;
    d.alpha = alpha;
    d.gradient = ScreenRectGradient::Vertical;
    d.blendMode = blend;
    AttachClipTo(d);
    d.paintOrder = params->NextUiPaintOrder();
    UiRectBucket(params, lateDepth, overlayDepth).PushBack(d);
}

void GuiPaintContext::FillRectGradientHorizontal(
        float x,
        float y,
        float w,
        float h,
        const Vector3& rgbLeft,
        const Vector3& rgbRight,
        float alpha,
        const SceneBlendMode blend) {
    if (params == nullptr || w <= 0.0F || h <= 0.0F || !ClipAllowsImmediateDraw()) {
        return;
    }
    ScreenRectDraw d{};
    d.x = x;
    d.y = y;
    d.width = w;
    d.height = h;
    d.color = rgbLeft;
    d.colorB = rgbRight;
    d.alpha = alpha;
    d.gradient = ScreenRectGradient::Horizontal;
    d.blendMode = blend;
    AttachClipTo(d);
    d.paintOrder = params->NextUiPaintOrder();
    UiRectBucket(params, lateDepth, overlayDepth).PushBack(d);
}

void GuiPaintContext::FillDropShadow(
        float x,
        float y,
        float w,
        float h,
        float offsetX,
        float offsetY,
        const Vector3& shadowRgb,
        float strength) {
    if (params == nullptr || w <= 0.0F || h <= 0.0F || strength <= 0.0F) {
        return;
    }
    constexpr int kLayers = 6;
    for (int i = kLayers; i >= 1; --i) {
        const float t = static_cast<float>(i) / static_cast<float>(kLayers);
        const float spread = 1.0F + 3.5F * t;
        const float ox = offsetX * t;
        const float oy = offsetY * t;
        const float layerAlpha =
                (0.045F + 0.11F * t) * std::min(strength, 1.5F);
        FillRect(
                x + ox - spread * 0.5F,
                y + oy - spread * 0.5F,
                w + spread,
                h + spread,
                shadowRgb,
                layerAlpha,
                SceneBlendMode::Multiply);
    }
}

void GuiPaintContext::StrokeRect(
        float x, float y, float w, float h, float thickness, const Vector3& rgb, float alpha) {
    if (params == nullptr || thickness <= 0.0F || w <= 0.0F || h <= 0.0F) {
        return;
    }
    FillRect(x, y, w, thickness, rgb, alpha);
    FillRect(x, y + h - thickness, w, thickness, rgb, alpha);
    FillRect(x, y, thickness, h, rgb, alpha);
    FillRect(x + w - thickness, y, thickness, h, rgb, alpha);
}

void GuiPaintContext::DrawText(
        float x,
        float y,
        float sizePixels,
        const Utf8String& text,
        const Vector3& rgb,
        float alpha,
        bool bold) {
    if (params == nullptr || text.IsEmpty() || !ClipAllowsImmediateDraw()) {
        return;
    }
    ScreenTextDraw d{};
    d.text = text;
    d.x = x;
    d.y = y;
    d.sizePixels = sizePixels;
    d.color = rgb;
    d.alpha = alpha;
    d.bold = bold;
    AttachClipTo(d);
    d.paintOrder = params->NextUiPaintOrder();
    UiTextBucket(params, lateDepth, overlayDepth).PushBack(Spark::MoveTemp(d));
}

void GuiPaintContext::DrawTextWrapped(
        const float x,
        const float y,
        const float maxWidth,
        const float sizePixels,
        const Utf8String& text,
        const Vector3& rgb,
        const float alpha,
        const bool bold) {
    if (params == nullptr || text.IsEmpty() || maxWidth <= 2.0F) {
        return;
    }
    Array<Utf8String> lines;
    BuildWrappedTextLines(layoutFont, text, sizePixels, maxWidth, lines);
    const float lineStep =
            layoutFont != nullptr ? layoutFont->GetLineSpacingPixels(sizePixels) : sizePixels * 1.2F;
    float cy = y;
    for (std::size_t i = 0; i < lines.GetSize(); ++i) {
        DrawText(x, cy, sizePixels, lines[i], rgb, alpha, bold);
        cy += lineStep;
    }
}

void GuiPaintContext::SetFramebufferPixelSize(float width, float height) noexcept {
    fbW = width > 0.0F ? width : 1.0F;
    fbH = height > 0.0F ? height : 1.0F;
    guiSpriteSortCounter = 50000;
}

void GuiPaintContext::EmitSprite(
        const Matrix4& modelWorld,
        const std::int32_t textureLayer,
        const Vector4& uvRect,
        const Vector4& tint,
        const std::int32_t sortOrder,
        const SceneBlendMode blend) {
    if (params == nullptr || textureLayer < 0) {
        return;
    }
    if (params->sprites.GetSize() >= SceneRenderParams::MaxSprites) {
        return;
    }
    SceneSpriteDraw sd{};
    sd.model = modelWorld;
    sd.tint = tint;
    sd.uvRect = uvRect;
    sd.textureLayer = textureLayer;
    sd.sortOrder = sortOrder >= 0 ? sortOrder : guiSpriteSortCounter++;
    sd.blendMode = blend;
    params->sprites.PushBack(sd);
}

std::int32_t GuiPaintContext::AcquireUiTextureLayer(const SharedPtr<Texture2D>& texture) {
    if (params == nullptr || !texture) {
        return -1;
    }
    for (std::size_t i = 0; i < params->uiTextures.GetSize(); ++i) {
        if (params->uiTextures[i].Get() == texture.Get()) {
            return static_cast<std::int32_t>(i);
        }
    }
    if (params->uiTextures.GetSize() >= SceneRenderParams::MaxUiTextures) {
        return -1;
    }
    params->uiTextures.PushBack(texture);
    return static_cast<std::int32_t>(params->uiTextures.GetSize() - 1U);
}

void GuiPaintContext::DrawSpriteSlice(
        const GuiSpriteSlice& slice,
        const float x,
        const float y,
        const float w,
        const float h,
        const Vector3& tint,
        const float alpha,
        const SceneBlendMode blend) {
    if (!slice.IsValid() || params == nullptr || w <= 0.0F || h <= 0.0F || !ClipAllowsImmediateDraw()) {
        return;
    }
    const std::int32_t layer = AcquireUiTextureLayer(slice.texture);
    if (layer < 0) {
        return;
    }
    const Texture2D* tex = slice.texture.Get();
    std::uint32_t atlasW = 1U;
    std::uint32_t atlasH = 1U;
    ComputeUiAtlasLayerExtent(params, atlasW, atlasH);
    ScreenSpriteDraw d{};
    d.x = x;
    d.y = y;
    d.width = w;
    d.height = h;
    d.uvRect = tex != nullptr ? ScaleUvToUiAtlasLayer(slice.uvRect, *tex, atlasW, atlasH) : slice.uvRect;
    d.textureLayer = layer;
    d.tint = tint;
    d.alpha = alpha;
    d.blendMode = blend;
    AttachClipTo(d);
    d.paintOrder = params->NextUiPaintOrder();
    UiSpriteBucket(params, lateDepth, overlayDepth).PushBack(d);
}

void GuiPaintContext::DrawNineSlice(
        const GuiSpriteSlice& slice,
        const float x,
        const float y,
        const float w,
        const float h,
        const Vector3& tint,
        const float alpha,
        const SceneBlendMode blend) {
    if (!slice.IsValid() || slice.nineSlice.IsEmpty()) {
        DrawSpriteSlice(slice, x, y, w, h, tint, alpha, blend);
        return;
    }
    if (params == nullptr || w <= 0.0F || h <= 0.0F || !ClipAllowsImmediateDraw()) {
        return;
    }
    const Texture2D* tex = slice.texture.Get();
    if (tex == nullptr) {
        return;
    }
    const float texW = static_cast<float>(std::max(1U, tex->GetWidth()));
    const float texH = static_cast<float>(std::max(1U, tex->GetHeight()));
    const float srcW = (slice.uvRect.z - slice.uvRect.x) * texW;
    const float srcH = (slice.uvRect.w - slice.uvRect.y) * texH;
    if (srcW <= 0.5F || srcH <= 0.5F) {
        DrawSpriteSlice(slice, x, y, w, h, tint, alpha, blend);
        return;
    }
    /** Border thickness stays at inset pixels (× UI scale), not stretched with dest/src ratio. */
    const float uiPx = GetLayoutMetrics().Scaled(1.0F);
    const float dl = slice.nineSlice.left * uiPx;
    const float dr = slice.nineSlice.right * uiPx;
    const float dt = slice.nineSlice.top * uiPx;
    const float db = slice.nineSlice.bottom * uiPx;
    const float centerW = std::max(0.0F, w - dl - dr);
    const float centerH = std::max(0.0F, h - dt - db);
    const float u0 = slice.uvRect.x;
    const float v0 = slice.uvRect.y;
    const float u1 = slice.uvRect.z;
    const float v1 = slice.uvRect.w;
    const float uMid0 = u0 + (slice.nineSlice.left / srcW) * (u1 - u0);
    const float uMid1 = u1 - (slice.nineSlice.right / srcW) * (u1 - u0);
    const float vMid0 = v0 + (slice.nineSlice.top / srcH) * (v1 - v0);
    const float vMid1 = v1 - (slice.nineSlice.bottom / srcH) * (v1 - v0);

    auto drawPart = [&](float px, float py, float pw, float ph, float pu0, float pv0, float pu1, float pv1) {
        if (pw <= 0.0F || ph <= 0.0F) {
            return;
        }
        GuiSpriteSlice part = slice;
        part.uvRect = {pu0, pv0, pu1, pv1};
        DrawSpriteSlice(part, px, py, pw, ph, tint, alpha, blend);
    };

    drawPart(x, y, dl, dt, u0, v0, uMid0, vMid0);
    drawPart(x + dl, y, centerW, dt, uMid0, v0, uMid1, vMid0);
    drawPart(x + dl + centerW, y, dr, dt, uMid1, v0, u1, vMid0);
    drawPart(x, y + dt, dl, centerH, u0, vMid0, uMid0, vMid1);
    drawPart(x + dl, y + dt, centerW, centerH, uMid0, vMid0, uMid1, vMid1);
    drawPart(x + dl + centerW, y + dt, dr, centerH, uMid1, vMid0, u1, vMid1);
    drawPart(x, y + dt + centerH, dl, db, u0, vMid1, uMid0, v1);
    drawPart(x + dl, y + dt + centerH, centerW, db, uMid0, vMid1, uMid1, v1);
    drawPart(x + dl + centerW, y + dt + centerH, dr, db, uMid1, vMid1, u1, v1);
}

}  // namespace Spark::Gui
