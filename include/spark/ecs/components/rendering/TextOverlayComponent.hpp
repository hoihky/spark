#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Screen-space text overlay (pixel coordinates, Y downward). Rendered after the 3D scene using the world's UI font.
 */
class TextOverlayComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TextOverlay;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    TextOverlayComponent() = default;

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

    SPARK_SCRIPT_BIND(get_text)
    [[nodiscard]] const Utf8String& GetText() const noexcept { return text; }
    void SetText(Utf8String value);

    SPARK_SCRIPT_BIND(get_screen_x)
    [[nodiscard]] float GetScreenX() const noexcept { return screenX; }
    SPARK_SCRIPT_BIND(get_screen_y)
    [[nodiscard]] float GetScreenY() const noexcept { return screenY; }
    void SetScreenPosition(float x, float y);

    SPARK_SCRIPT_BIND(get_font_size_pixels)
    [[nodiscard]] float GetFontSizePixels() const noexcept { return fontSizePixels; }
    void SetFontSizePixels(float px);

    SPARK_SCRIPT_BIND(get_color)
    [[nodiscard]] const Vector3& GetColor() const noexcept { return color; }
    void SetColor(const Vector3& rgb);
    SPARK_SCRIPT_BIND(get_alpha)
    [[nodiscard]] float GetAlpha() const noexcept { return alpha; }
    void SetAlpha(float a);

    SPARK_SCRIPT_BIND(is_visible)
    [[nodiscard]] bool IsVisible() const noexcept { return visible; }
    void SetVisible(bool v);

private:
    Utf8String text{};
    float screenX = 8.0F;
    float screenY = 8.0F;
    float fontSizePixels = 18.0F;
    Vector3 color{1.0F, 1.0F, 1.0F};
    float alpha = 1.0F;
    bool visible = true;
};

}  // namespace Spark
