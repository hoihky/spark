#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/**
 * Media-style tile: elevated card, placeholder "cover" block, bold title + subtitle.
 * Screen-space only (no texture sampling yet); use for launcher rows or library-style layouts.
 */
class AlbumCard final : public Widget {
public:
    void SetTitle(Utf8String t) { title = Spark::MoveTemp(t); }
    void SetSubtitle(Utf8String s) { subtitle = Spark::MoveTemp(s); }
    void SetTitleFontSize(float px) noexcept { titleFontPx = px; }
    void SetSubtitleFontSize(float px) noexcept { subtitleFontPx = px; }
    /** Thin accent strip on the left when true. */
    void SetAccentRailEnabled(bool v) noexcept { accentRail = v; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;

private:
    Utf8String title{};
    Utf8String subtitle{};
    float titleFontPx = 26.0F;
    float subtitleFontPx = 18.0F;
    bool accentRail = false;
    Rect artRect{};
    Rect titleRect{};
    Rect subtitleRect{};
};

}  // namespace Spark::Gui
