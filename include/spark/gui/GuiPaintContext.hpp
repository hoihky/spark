#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

namespace Spark {

struct SceneRenderParams;
struct ScreenRectDraw;
struct ScreenTextDraw;
class Font;

namespace Gui {

struct GuiTheme;
struct GuiLayoutMetrics;
class Widget;

/**
 * Accumulates immediate-mode style draw commands into SceneRenderParams (solid rects + text).
 * Main lists are drawn as solid rects then text; overlay then late layers draw after (see PushOverlayLayer /
 * PushLateLayer).
 */
class GuiPaintContext {
public:
    explicit GuiPaintContext(SceneRenderParams& sceneParams) noexcept : params(&sceneParams) {}

    /** Optional UI font for layout (wrapping); set from <c>PaintGuiCanvases</c> when available. */
    void SetLayoutFont(const Font* font) noexcept { layoutFont = font; }
    [[nodiscard]] const Font* GetLayoutFont() const noexcept { return layoutFont; }

    /** Optional: set each frame so controls can highlight hover/active/focus. */
    void SetInteraction(Widget* hot, Widget* active, Widget* focus) noexcept {
        hotWidget = hot;
        activeWidget = active;
        focusWidget = focus;
    }

    /** Bound per canvas in `PaintGuiCanvases`; never null when painting engine canvases. */
    void SetTheme(const GuiTheme* theme) noexcept { themePtr = theme; }
    [[nodiscard]] const GuiTheme& GetTheme() const noexcept;

    void SetLayoutMetrics(const GuiLayoutMetrics* metrics) noexcept { metricsPtr = metrics; }
    [[nodiscard]] const GuiLayoutMetrics& GetLayoutMetrics() const noexcept;

    [[nodiscard]] bool IsHot(const Widget* self) const noexcept { return hotWidget == self; }
    [[nodiscard]] bool IsActive(const Widget* self) const noexcept { return activeWidget == self; }
    [[nodiscard]] bool IsFocused(const Widget* self) const noexcept { return focusWidget == self; }

    void FillRect(
            float x,
            float y,
            float w,
            float h,
            const Vector3& rgb,
            float alpha,
            SceneBlendMode blend = kSceneBlendModeDefault);
    void FillRectGradientVertical(
            float x,
            float y,
            float w,
            float h,
            const Vector3& rgbTop,
            const Vector3& rgbBottom,
            float alpha,
            SceneBlendMode blend = kSceneBlendModeDefault);
    void FillRectGradientHorizontal(
            float x,
            float y,
            float w,
            float h,
            const Vector3& rgbLeft,
            const Vector3& rgbRight,
            float alpha,
            SceneBlendMode blend = kSceneBlendModeDefault);
    /** Layered offset rects for a soft 2D drop shadow (draw before the main control). */
    void FillDropShadow(
            float x,
            float y,
            float w,
            float h,
            float offsetX,
            float offsetY,
            const Vector3& shadowRgb,
            float strength = 1.0F);
    void StrokeRect(float x, float y, float w, float h, float thickness, const Vector3& rgb, float alpha);
    /** Filled rounded rect, solid color (radius from theme / caller). */
    void FillRoundRectSolid(
            float x, float y, float w, float h, float cornerRadius, const Vector3& rgb, float alpha);
    /** Vertical gradient fill with uniform corner rounding (see <c>GuiTheme::controlCornerRadius</c>). */
    void FillRoundRectGradientVertical(
            float x,
            float y,
            float w,
            float h,
            float cornerRadius,
            const Vector3& rgbTop,
            const Vector3& rgbBottom,
            float alpha);
    /** Axis-aligned stroke (corners square); pair with rounded fills for a clean frame. */
    void StrokeRoundRect(
            float x, float y, float w, float h, float cornerRadius, float thickness, const Vector3& rgb, float alpha);
    void DrawText(
            float x,
            float y,
            float sizePixels,
            const Utf8String& text,
            const Vector3& rgb,
            float alpha,
            bool bold = false);

    /** Width in framebuffer pixels (uses layout font when set). */
    [[nodiscard]] float MeasureUtf8Width(const Utf8String& text, float sizePixels) const noexcept;

    /**
     * Word-wraps at ASCII spaces to fit maxWidth (when layout font is set); otherwise uses a fixed
     * character-per-line estimate. Each line is emitted as a separate screen-text draw.
     */
    void DrawTextWrapped(
            float x,
            float y,
            float maxWidth,
            float sizePixels,
            const Utf8String& text,
            const Vector3& rgb,
            float alpha,
            bool bold = false);

    /**
     * Lays out text inside a rectangle: optional word wrap, ellipsis, and scissor clipping.
     * Single-line mode vertically centers when <c>rect.height</c> exceeds the font size.
     */
    void DrawTextInRect(
            const Rect& rect,
            float sizePixels,
            const Utf8String& text,
            const Vector3& rgb,
            float alpha,
            bool bold,
            TextLayout layout);

    /** Truncates UTF-8 text to fit <c>maxWidth</c> pixels, appending an ellipsis when shortened. */
    [[nodiscard]] Utf8String EllipsizeUtf8(const Utf8String& text, float sizePixels, float maxWidth) const;

    [[nodiscard]] float GetLineSpacingPixels(float sizePixels) const noexcept;

    /** Emits wrapped lines (same algorithm as <c>DrawTextWrapped</c>). */
    void BuildWrappedLines(
            const Utf8String& text, float sizePixels, float maxWidth, Array<Utf8String>& outLines) const;

    /** Intersects with the current clip (if any). Must be paired with PopClipRect. */
    void PushClipRect(float x, float y, float width, float height);
    void PopClipRect() noexcept;
    /** Clears the clip stack (call at the start of each canvas paint). */
    void ClearClipStack() noexcept { clipStack.Clear(); }

    /**
     * Routes rects/text to <c>screenOverlayRects</c> / <c>screenOverlayTexts</c> (drawn after all main UI text).
     * Clears the clip stack so parent scroll viewports do not clip popovers; use <c>PushClipRect</c> inside the
     * overlay for local scissoring (e.g. dropdown list viewport).
     */
    void PushOverlayLayer() noexcept {
        clipStack.Clear();
        ++overlayDepth;
    }
    void PopOverlayLayer() noexcept;
    void ResetOverlayLayer() noexcept { overlayDepth = 0; }

    /**
     * Routes rects/text to <c>screenLateRects</c> / <c>screenLateTexts</c> (drawn after overlay). Prefer for
     * dropdown lists so the destination framebuffer already holds the full composed main + overlay UI.
     */
    void PushLateLayer() noexcept {
        clipStack.Clear();
        ++lateDepth;
    }
    void PopLateLayer() noexcept;
    void ResetLateLayer() noexcept { lateDepth = 0; }

    /** Call once per frame before painting GUI (e.g. from <c>PaintGuiCanvases</c>) for sprite helpers. */
    void SetFramebufferPixelSize(float width, float height) noexcept;
    [[nodiscard]] float GetFramebufferWidth() const noexcept { return fbW; }
    [[nodiscard]] float GetFramebufferHeight() const noexcept { return fbH; }
    /**
     * Appends a lit sprite (same pass as world sprites). Requires <c>SceneRenderParams::viewProjection</c> to map
     * world XY consistently with these units (pixel HUD: ortho <c>0..fbW</c> × <c>0..fbH</c> with identity view).
     */
    void EmitSprite(
            const Matrix4& modelWorld,
            std::int32_t textureLayer,
            const Vector4& uvRect,
            const Vector4& tint,
            std::int32_t sortOrder = -1,
            SceneBlendMode blend = kSceneBlendModeDefault);

    Widget* hotWidget = nullptr;
    Widget* activeWidget = nullptr;
    Widget* focusWidget = nullptr;

private:
    [[nodiscard]] bool CurrentClipAllowsDraw() const noexcept;
    /** Main layer respects clip stack; overlay/late skip degenerate parent clips but honor local PushClipRect via AttachClipTo. */
    [[nodiscard]] bool ClipAllowsImmediateDraw() const noexcept;
    void AttachClipTo(ScreenRectDraw& d) const noexcept;
    void AttachClipTo(ScreenTextDraw& d) const noexcept;

    SceneRenderParams* params = nullptr;
    const Font* layoutFont = nullptr;
    const GuiTheme* themePtr = nullptr;
    const GuiLayoutMetrics* metricsPtr = nullptr;
    Array<Rect> clipStack{};
    int overlayDepth = 0;
    int lateDepth = 0;
    float fbW = 1.0F;
    float fbH = 1.0F;
    int guiSpriteSortCounter = 50000;
};

}  // namespace Gui
}  // namespace Spark
