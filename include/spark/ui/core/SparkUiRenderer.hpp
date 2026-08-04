#pragma once

#include "spark/ui/core/UiPaintContext.hpp"
#include "spark/ui/core/UiTheme.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/core/UiTheme.hpp"

namespace Spark {

struct SceneRenderParams;

namespace Ui {

/**
 * Spark-native renderer: adapts <c>IUiRenderer</c> to <c>UiPaintContext</c> so UI draws through the
 * existing Vulkan <c>VulkanScreenUiPass</c> path (screen rects + text over 2D/3D scene).
 */
class SparkUiRenderer final : public IUiRenderer {
public:
    explicit SparkUiRenderer(SceneRenderParams& sceneParams) noexcept;

    void SetTheme(const UiTheme* themeIn) noexcept override;
    [[nodiscard]] const UiTheme& GetTheme() const noexcept override;

    void SetLayoutFont(const Font* font) noexcept override;
    void SetLayoutMetrics(const UiLayoutMetrics* metricsIn) noexcept override;
    [[nodiscard]] const UiLayoutMetrics& GetLayoutMetrics() const noexcept override;

    void SetInteraction(IUiElement* hot, IUiElement* active, IUiElement* focus) noexcept override;
    [[nodiscard]] bool IsHot(const IUiElement* element) const noexcept override;
    [[nodiscard]] bool IsActive(const IUiElement* element) const noexcept override;
    [[nodiscard]] bool IsFocused(const IUiElement* element) const noexcept override;

    void FillRect(
            float x,
            float y,
            float w,
            float h,
            const Vector3& rgb,
            float alpha,
            SceneBlendMode blend) override;

    void FillRectGradientVertical(
            float x,
            float y,
            float w,
            float h,
            const Vector3& rgbTop,
            const Vector3& rgbBottom,
            float alpha,
            SceneBlendMode blend) override;

    void FillDropShadow(
            float x,
            float y,
            float w,
            float h,
            float offsetX,
            float offsetY,
            const Vector3& rgb,
            float alpha) override;

    void FillRoundRectGradientVertical(
            float x,
            float y,
            float w,
            float h,
            float cornerRadius,
            const Vector3& rgbTop,
            const Vector3& rgbBottom,
            float alpha,
            SceneBlendMode blend) override;

    void StrokeRect(
            float x,
            float y,
            float w,
            float h,
            float strokeWidth,
            const Vector3& rgb,
            float alpha) override;

    void StrokeRoundRect(
            float x,
            float y,
            float w,
            float h,
            float cornerRadius,
            float strokeWidth,
            const Vector3& rgb,
            float alpha) override;

    void DrawText(
            float x,
            float y,
            float maxWidth,
            Utf8StringView text,
            const Vector3& rgb,
            float alpha,
            float fontSizePx,
            bool bold) override;

    void DrawTextInRect(
            const Rect& rect,
            float fontSizePx,
            Utf8StringView text,
            const Vector3& rgb,
            float alpha,
            bool bold,
            TextLayout layout) override;

    [[nodiscard]] Utf8String EllipsizeUtf8(Utf8StringView text, float fontSizePx, float maxWidth) const override;

    void PushClip(const Rect& rect) override;
    void PopClip() override;

    void PushOverlayLayer() override;
    void PushLateLayer() override;

    [[nodiscard]] UiPaintContext& GetPaintContext() noexcept { return paintContext; }

private:
    UiPaintContext paintContext;
    UiTheme fallbackTheme{UiTheme::ClassicMint()};
    UiLayoutMetrics fallbackMetrics{UiLayoutMetrics::Default()};
    const UiTheme* themePtr = nullptr;
    const UiLayoutMetrics* metricsPtr = nullptr;
    IUiElement* hotElement = nullptr;
    IUiElement* activeElement = nullptr;
    IUiElement* focusElement = nullptr;
};

}  // namespace Ui
}  // namespace Spark
