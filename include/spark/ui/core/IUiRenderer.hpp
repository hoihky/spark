#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark {

class Font;

namespace Ui {

class IUiElement;
struct UiLayoutMetrics;
struct UiTheme;

/**
 * Backend-neutral draw surface. Spark implementation records into <c>SceneRenderParams</c>
 * for the Vulkan screen UI pass (2D overlay on top of 3D scene).
 */
class IUiRenderer {
public:
    virtual ~IUiRenderer() = default;

    virtual void SetTheme(const UiTheme* theme) noexcept = 0;
    [[nodiscard]] virtual const UiTheme& GetTheme() const noexcept = 0;

    virtual void SetLayoutFont(const Font* font) noexcept = 0;
    virtual void SetLayoutMetrics(const UiLayoutMetrics* metrics) noexcept = 0;
    [[nodiscard]] virtual const UiLayoutMetrics& GetLayoutMetrics() const noexcept = 0;

    virtual void SetInteraction(IUiElement* hot, IUiElement* active, IUiElement* focus) noexcept = 0;
    [[nodiscard]] virtual bool IsHot(const IUiElement* element) const noexcept = 0;
    [[nodiscard]] virtual bool IsActive(const IUiElement* element) const noexcept = 0;
    [[nodiscard]] virtual bool IsFocused(const IUiElement* element) const noexcept = 0;

    virtual void FillRect(
            float x,
            float y,
            float w,
            float h,
            const Vector3& rgb,
            float alpha,
            SceneBlendMode blend = kSceneBlendModeDefault) = 0;

    virtual void FillRectGradientVertical(
            float x,
            float y,
            float w,
            float h,
            const Vector3& rgbTop,
            const Vector3& rgbBottom,
            float alpha,
            SceneBlendMode blend = kSceneBlendModeDefault) = 0;

    virtual void FillDropShadow(
            float x,
            float y,
            float w,
            float h,
            float offsetX,
            float offsetY,
            const Vector3& rgb,
            float alpha) = 0;

    virtual void FillRoundRectGradientVertical(
            float x,
            float y,
            float w,
            float h,
            float cornerRadius,
            const Vector3& rgbTop,
            const Vector3& rgbBottom,
            float alpha,
            SceneBlendMode blend = kSceneBlendModeDefault) = 0;

    virtual void StrokeRect(
            float x,
            float y,
            float w,
            float h,
            float strokeWidth,
            const Vector3& rgb,
            float alpha) = 0;

    virtual void StrokeRoundRect(
            float x,
            float y,
            float w,
            float h,
            float cornerRadius,
            float strokeWidth,
            const Vector3& rgb,
            float alpha) = 0;

    virtual void DrawText(
            float x,
            float y,
            float maxWidth,
            Utf8StringView text,
            const Vector3& rgb,
            float alpha,
            float fontSizePx,
            bool bold = false) = 0;

    virtual void DrawTextInRect(
            const Rect& rect,
            float fontSizePx,
            Utf8StringView text,
            const Vector3& rgb,
            float alpha,
            bool bold,
            TextLayout layout) = 0;

    [[nodiscard]] virtual Utf8String EllipsizeUtf8(Utf8StringView text, float fontSizePx, float maxWidth) const = 0;

    virtual void PushClip(const Rect& rect) = 0;
    virtual void PopClip() = 0;

    virtual void PushOverlayLayer() = 0;
    virtual void PushLateLayer() = 0;
};

}  // namespace Ui
}  // namespace Spark
