#pragma once

#include "spark/gui/GuiSpriteSlice.hpp"
#include "spark/gui/GuiSkinElement.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/**
 * Displays a static or skin-driven sprite region. Use for HUD art, inventory icons, or decorative UI.
 */
class Image final : public Widget {
public:
    void SetSlice(GuiSpriteSlice slice) { spriteSlice = MoveTemp(slice); }
    void SetSkinElement(GuiSkinElement element) noexcept { skinElement = element; useSkinElement = true; }
    void ClearSkinElement() noexcept { useSkinElement = false; }
    void SetTint(const Vector3& rgb) noexcept { tint = rgb; }
    void SetAlpha(float a) noexcept { alpha = a; }
    void SetPreserveAspect(bool v) noexcept { preserveAspect = v; }
    /** Centers a fixed-size sprite inside the layout slot (e.g. toolbar icons). */
    void SetPreferredSize(float w, float h) noexcept {
        preferredW = w;
        preferredH = h;
        usePreferredSize = true;
    }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    GuiSpriteSlice spriteSlice{};
    GuiSkinElement skinElement = GuiSkinElement::PanelBackground;
    bool useSkinElement = false;
    Vector3 tint{1.0F, 1.0F, 1.0F};
    float alpha = 1.0F;
    bool preserveAspect = true;
    bool usePreferredSize = false;
    float preferredW = 48.0F;
    float preferredH = 48.0F;
};

}  // namespace Spark::Gui
