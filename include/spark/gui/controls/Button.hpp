#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"

#include <functional>

namespace Spark {

namespace Gui {

class GuiPaintContext;

/** Clickable button with centered label; optional UTF-8 icon glyph and optional texture (see SetIconTexture). */
class Button final : public Widget {
public:
    void SetLabel(Utf8String t) { label = Spark::MoveTemp(t); }
    void SetOnClick(std::function<void()> fn) { onClick = Spark::MoveTemp(fn); }
    /** When set, <c>NotifyClick</c> invokes this (with modifiers) instead of <c>SetOnClick</c>. */
    void SetOnClickWithFrame(std::function<void(const GuiFrameInput&, GuiCanvasComponent&)> fn) {
        onClickWithFrame = Spark::MoveTemp(fn);
    }
    void ClearOnClickWithFrame() noexcept { onClickWithFrame = nullptr; }
    void SetFontSize(float px) noexcept { fontPx = px; }
    void SetLabelBold(bool b) noexcept { labelBold = b; }
    /** Optional leading symbol (emoji or icon font); empty = none. */
    void SetIconGlyph(Utf8String g) { iconGlyph = Spark::MoveTemp(g); }
    void ClearIconGlyph() noexcept { iconGlyph.Clear(); }
    void SetIconGlyphSize(float px) noexcept { iconGlyphPx = px; }
    /** Bitmap icon slot (rounded tile); full GPU blit can be wired later via sprite pass. */
    void SetIconTexture(SharedPtr<Texture2D> tex) { iconTexture = Spark::MoveTemp(tex); }
    void ClearIconTexture() noexcept { iconTexture.Reset(); }
    /** Highlighted style (e.g. list row selection). */
    void SetAccentSelected(bool v) noexcept { accentSelected = v; }
    /** Solid fill, no drop shadow — avoids see-through rows over 3D (e.g. scene editor lists). */
    void SetOpaqueSurface(bool v) noexcept { opaqueSurface = v; }

    void Paint(GuiPaintContext& ctx) const override;
    void NotifyClick(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    Utf8String label{Utf8String("Button")};
    Utf8String iconGlyph{};
    float fontPx = 22.0F;
    float iconGlyphPx = 20.0F;
    bool labelBold = false;
    bool accentSelected = false;
    bool opaqueSurface = false;
    SharedPtr<Texture2D> iconTexture{};
    std::function<void()> onClick{};
    std::function<void(const GuiFrameInput&, GuiCanvasComponent&)> onClickWithFrame{};
};

}  // namespace Gui
}  // namespace Spark
