#pragma once

#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/core/UiTheme.hpp"

namespace Spark {

class IImGuiLayer;
class Window;
class IInput;

namespace Ui {

struct UiFrameContext;

/** How an ImGui panel window is positioned each frame. */
enum class ImguiPanelPlacement : std::uint8_t {
    /** Default: place once, user can drag and resize afterward. */
    Movable = 0,
    /** Center on first show, then user can drag and resize. */
    CenterOnce = 1,
    /** Pin to layout bounds every frame (editor-style fixed panels). */
    LockedSide = 2,
};

class ImguiUiRenderer final : public IUiRenderer {
public:
    explicit ImguiUiRenderer(IImGuiLayer* layer) noexcept;

    void SetImGuiLayer(IImGuiLayer* layer) noexcept;

    void BeginBackendFrame(Window& window, IInput& input, const UiFrameContext& context);
    void EndBackendFrame();

    void SetTheme(const UiTheme* theme) noexcept override;
    [[nodiscard]] const UiTheme& GetTheme() const noexcept override;

    void SetLayoutFont(const Font* font) noexcept override;
    void SetLayoutMetrics(const UiLayoutMetrics* metrics) noexcept override;
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

    [[nodiscard]] bool WantsCaptureMouse() const noexcept;
    [[nodiscard]] bool WantsCaptureKeyboard() const noexcept;

    /** Dear ImGui immediate widgets (Phase 3). No-ops when <c>SPARK_ENABLE_IMGUI</c> is off. */
    void TextUnformatted(Utf8StringView text);
    void TextDisabled(Utf8StringView text);
    void Separator();

    [[nodiscard]] bool Button(const char* id, Utf8StringView label);
    [[nodiscard]] bool Checkbox(const char* id, Utf8StringView label, bool& value);
    [[nodiscard]] bool SliderFloat(
            const char* id,
            Utf8StringView label,
            float& value,
            float minValue,
            float maxValue);

    /** Opens an ImGui window sized/positioned from <c>bounds</c>. Returns false only when Begin was not called. */
    [[nodiscard]] bool BeginPanel(
            const char* id,
            Utf8StringView title,
            bool* open,
            const Rect& bounds,
            ImguiPanelPlacement placement = ImguiPanelPlacement::Movable);
    void EndPanel();

    [[nodiscard]] bool BeginScrollRegion(const char* id, float height);
    void EndScrollRegion();
    void SetScrollY(float y);
    [[nodiscard]] float GetScrollY() const noexcept;

    /** Full-bleed dock host inside <c>bounds</c>. Child panels dock via matching window titles. */
    [[nodiscard]] bool BeginDockWorkspace(
            const char* id,
            const Rect& bounds,
            float leftWidth,
            float rightWidth,
            const char* leftWindowName,
            const char* centerWindowName,
            const char* rightWindowName,
            bool& layoutBuilt);
    void EndDockWorkspace();

private:
    IImGuiLayer* imguiLayer = nullptr;
    UiTheme fallbackTheme{UiTheme::ClassicMint()};
    UiLayoutMetrics fallbackMetrics{UiLayoutMetrics::Default()};
    const UiTheme* themePtr = nullptr;
    const UiLayoutMetrics* metricsPtr = nullptr;
    int panelStack = 0;
    int scrollStack = 0;
    float activeScrollY = 0.0F;
};

}  // namespace Ui
}  // namespace Spark
