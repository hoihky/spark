#pragma once

#include "spark/gui/Widget.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/** Solid background container; children share the same bounds unless a subclass stacks them. */
class Panel final : public Widget {
public:
    void SetBackgroundColor(const Vector3& rgb, float a) noexcept {
        bg = rgb;
        bg2 = rgb;
        useGradient = false;
        bgAlpha = a;
    }
    void SetBackgroundGradient(const Vector3& rgbTop, const Vector3& rgbBottom, float a) noexcept {
        bg = rgbTop;
        bg2 = rgbBottom;
        useGradient = true;
        bgAlpha = a;
    }
    void SetPadding(float p) noexcept { padding = p; }
    void SetDropShadowEnabled(bool enabled) noexcept { dropShadow = enabled; }
    /** When false, no border stroke (e.g. full-screen backdrops). */
    void SetChromeEnabled(bool enabled) noexcept { chrome = enabled; }
    /** When false, only children are painted (parent may supply an opaque backdrop). */
    void SetBackgroundEnabled(bool enabled) noexcept { backgroundEnabled = enabled; }
    /**
     * When true, background uses <c>GuiTheme::dropdownPanel*</c> each paint (no stored gradient mutation).
     * Use for open dropdown list panels so theme updates do not overwrite sibling panel colors.
     */
    void SetDropdownListThemeBound(bool bound) noexcept { dropdownListThemeBound = bound; }
    /** When true and the canvas has a <c>GuiSkin</c>, paints a nine-slice panel background from the skin. */
    void SetPreferSkinBackground(bool v) noexcept { preferSkinBackground = v; }
    [[nodiscard]] bool GetPreferSkinBackground() const noexcept { return preferSkinBackground; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    Vector3 bg{0.16F, 0.22F, 0.18F};
    Vector3 bg2{0.16F, 0.22F, 0.18F};
    float bgAlpha = 0.94F;
    float padding = 8.0F;
    bool useGradient = false;
    bool dropShadow = true;
    bool chrome = true;
    bool backgroundEnabled = true;
    bool dropdownListThemeBound = false;
    bool preferSkinBackground = false;
};

}  // namespace Spark::Gui
