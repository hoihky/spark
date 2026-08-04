#include "spark/ui/core/SparkUiRenderer.hpp"

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/text/Font.hpp"

namespace Spark::Ui {

SparkUiRenderer::SparkUiRenderer(SceneRenderParams& sceneParams) noexcept : paintContext(sceneParams) {
    themePtr = &fallbackTheme;
    metricsPtr = &fallbackMetrics;
    paintContext.SetTheme(themePtr);
}

void SparkUiRenderer::SetTheme(const UiTheme* themeIn) noexcept {
    themePtr = themeIn != nullptr ? themeIn : &fallbackTheme;
    paintContext.SetTheme(themePtr);
}

const UiTheme& SparkUiRenderer::GetTheme() const noexcept {
    return themePtr != nullptr ? *themePtr : fallbackTheme;
}

void SparkUiRenderer::SetLayoutFont(const Font* font) noexcept {
    paintContext.SetLayoutFont(font);
}

void SparkUiRenderer::SetLayoutMetrics(const UiLayoutMetrics* metricsIn) noexcept {
    metricsPtr = metricsIn != nullptr ? metricsIn : &fallbackMetrics;
    paintContext.SetLayoutMetrics(metricsPtr);
}

const UiLayoutMetrics& SparkUiRenderer::GetLayoutMetrics() const noexcept {
    return metricsPtr != nullptr ? *metricsPtr : fallbackMetrics;
}

void SparkUiRenderer::SetInteraction(
        IUiElement* hot,
        IUiElement* active,
        IUiElement* focus) noexcept {
    hotElement = hot;
    activeElement = active;
    focusElement = focus;
}

bool SparkUiRenderer::IsHot(const IUiElement* element) const noexcept {
    return element != nullptr && hotElement == element;
}

bool SparkUiRenderer::IsActive(const IUiElement* element) const noexcept {
    return element != nullptr && activeElement == element;
}

bool SparkUiRenderer::IsFocused(const IUiElement* element) const noexcept {
    return element != nullptr && focusElement == element;
}

void SparkUiRenderer::FillRect(
        const float x,
        const float y,
        const float w,
        const float h,
        const Vector3& rgb,
        const float alpha,
        const SceneBlendMode blend) {
    paintContext.FillRect(x, y, w, h, rgb, alpha, blend);
}

void SparkUiRenderer::FillRectGradientVertical(
        const float x,
        const float y,
        const float w,
        const float h,
        const Vector3& rgbTop,
        const Vector3& rgbBottom,
        const float alpha,
        const SceneBlendMode blend) {
    paintContext.FillRectGradientVertical(x, y, w, h, rgbTop, rgbBottom, alpha, blend);
}

void SparkUiRenderer::FillDropShadow(
        const float x,
        const float y,
        const float w,
        const float h,
        const float offsetX,
        const float offsetY,
        const Vector3& rgb,
        const float alpha) {
    paintContext.FillDropShadow(x, y, w, h, offsetX, offsetY, rgb, alpha);
}

void SparkUiRenderer::FillRoundRectGradientVertical(
        const float x,
        const float y,
        const float w,
        const float h,
        const float cornerRadius,
        const Vector3& rgbTop,
        const Vector3& rgbBottom,
        const float alpha,
        const SceneBlendMode blend) {
    (void)blend;
    paintContext.FillRoundRectGradientVertical(x, y, w, h, cornerRadius, rgbTop, rgbBottom, alpha);
}

void SparkUiRenderer::StrokeRect(
        const float x,
        const float y,
        const float w,
        const float h,
        const float strokeWidth,
        const Vector3& rgb,
        const float alpha) {
    paintContext.StrokeRect(x, y, w, h, strokeWidth, rgb, alpha);
}

void SparkUiRenderer::StrokeRoundRect(
        const float x,
        const float y,
        const float w,
        const float h,
        const float cornerRadius,
        const float strokeWidth,
        const Vector3& rgb,
        const float alpha) {
    paintContext.StrokeRoundRect(x, y, w, h, cornerRadius, strokeWidth, rgb, alpha);
}

void SparkUiRenderer::DrawText(
        const float x,
        const float y,
        const float maxWidth,
        const Utf8StringView text,
        const Vector3& rgb,
        const float alpha,
        const float fontSizePx,
        const bool bold) {
    Utf8String drawText{text.CStr()};
    if (maxWidth > 2.0F) {
        drawText = paintContext.EllipsizeUtf8(drawText, fontSizePx, maxWidth);
    }
    paintContext.DrawText(x, y, fontSizePx, drawText, rgb, alpha, bold);
}

void SparkUiRenderer::DrawTextInRect(
        const Rect& rect,
        const float fontSizePx,
        const Utf8StringView text,
        const Vector3& rgb,
        const float alpha,
        const bool bold,
        const TextLayout layout) {
    paintContext.DrawTextInRect(rect, fontSizePx, Utf8String(text.CStr()), rgb, alpha, bold, layout);
}

Utf8String SparkUiRenderer::EllipsizeUtf8(const Utf8StringView text, const float fontSizePx, const float maxWidth) const {
    return paintContext.EllipsizeUtf8(Utf8String(text.CStr()), fontSizePx, maxWidth);
}

void SparkUiRenderer::PushClip(const Rect& rect) {
    paintContext.PushClipRect(rect.x, rect.y, rect.width, rect.height);
}

void SparkUiRenderer::PopClip() {
    paintContext.PopClipRect();
}

void SparkUiRenderer::PushOverlayLayer() {
    paintContext.PushOverlayLayer();
}

void SparkUiRenderer::PushLateLayer() {
    paintContext.PushLateLayer();
}

}  // namespace Spark::Ui
