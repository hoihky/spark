#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark::Gui {

/**
 * Semantic colors for built-in widgets. Games set a palette on `GuiCanvasComponent`; each frame
 * `PaintGuiCanvases` binds it to `GuiPaintContext` so `Widget::Paint` reads `ctx.GetTheme()`.
 *
 * `ClassicMint()` is the default mint/sage demo palette used across built-in demos.
 */
struct GuiTheme {
    Vector3 controlIdleTop{};
    Vector3 controlIdleBottom{};
    Vector3 controlHotTop{};
    Vector3 controlHotBottom{};
    Vector3 controlActiveTop{};
    Vector3 controlActiveBottom{};
    Vector3 controlAccentTop{};
    Vector3 controlAccentBottom{};
    float controlFillAlpha = 0.93F;
    float controlStrokeAlpha = 0.62F;
    /** Rounded corners for buttons and similar (pixels; clamped per widget size). */
    float controlCornerRadius = 8.0F;
    /** Rounded corners for single-line text fields. */
    float textBoxCornerRadius = 8.0F;

    Vector3 borderRgb{};
    Vector3 shadowRgb{};

    Vector3 labelPrimary{};
    Vector3 labelMuted{};
    /** Text on accent-filled controls (e.g. selected tab button). */
    Vector3 labelOnAccent{};

    Vector3 scrollViewportTop{};
    Vector3 scrollViewportBottom{};
    float scrollViewportAlpha = 0.94F;

    Vector3 insetTrackRgb{};
    Vector3 thumbGradientTop{};
    Vector3 thumbGradientBottom{};

    Vector3 sliderTrackRgb{};
    Vector3 sliderThumbTop{};
    Vector3 sliderThumbBottom{};

    Vector3 progressTrackRgb{};
    Vector3 progressFillTop{};
    Vector3 progressFillBottom{};

    Vector3 switchTrackOffTop{};
    Vector3 switchTrackOffBottom{};
    Vector3 switchTrackOnTop{};
    Vector3 switchTrackOnBottom{};
    Vector3 switchKnobTop{};
    Vector3 switchKnobBottom{};

    Vector3 checkFrameTop{};
    Vector3 checkFrameBottom{};
    Vector3 checkFillTop{};
    Vector3 checkFillBottom{};
    Vector3 checkInnerStrokeRgb{};

    Vector3 textBoxFillTop{};
    Vector3 textBoxFillBottom{};
    float textBoxFillAlpha = 0.88F;
    Vector3 textBoxBorderFocus{};
    Vector3 textBoxBorderIdle{};

    Vector3 numericFillTop{};
    Vector3 numericFillBottom{};
    Vector3 numericBorderDragging{};
    Vector3 numericBorderIdle{};

    Vector3 dialogDimmerTop{};
    Vector3 dialogDimmerBottom{};
    float dialogDimmerAlpha = 0.58F;
    Vector3 dialogTitleText{};
    Vector3 panelElevatedTop{};
    Vector3 panelElevatedBottom{};
    float panelElevatedAlpha = 0.94F;

    Vector3 tabHeaderTop{};
    Vector3 tabHeaderBottom{};
    float tabHeaderAlpha = 0.92F;
    Vector3 tabBodyTop{};
    Vector3 tabBodyBottom{};
    float tabBodyAlpha = 0.88F;

    Vector3 dropdownPanelTop{};
    Vector3 dropdownPanelBottom{};
    float dropdownPanelAlpha = 0.96F;

    /** Full-screen demo/menu backdrop behind root panels. */
    Vector3 shellBackdropTop{};
    Vector3 shellBackdropBottom{};
    float shellBackdropAlpha = 0.88F;

    /** `AlbumCard` / media tiles: elevated card + art placeholder. */
    Vector3 albumCardTop{};
    Vector3 albumCardBottom{};
    float albumCardAlpha = 0.98F;
    Vector3 albumArtPlaceholderTop{};
    Vector3 albumArtPlaceholderBottom{};
    Vector3 albumAccentBarRgb{};
    float albumAccentBarAlpha = 1.0F;

    [[nodiscard]] static GuiTheme ClassicMint() noexcept;
    /** Dark slate with teal accents — polished alternative for built-in demos. */
    [[nodiscard]] static GuiTheme TwilightSlate() noexcept;
    /** Dark blue-gray palette for the 3D scene editor sidebar (matches editor panel gradient). */
    [[nodiscard]] static GuiTheme SceneEditorDark() noexcept;
    /** White surfaces, black text — Windows high-contrast light style. */
    [[nodiscard]] static GuiTheme HighContrastLight() noexcept;
    /** Black surfaces, white text — Windows high-contrast dark style. */
    [[nodiscard]] static GuiTheme HighContrastDark() noexcept;
    /** Black surfaces with yellow accent text and focus rings. */
    [[nodiscard]] static GuiTheme HighContrastYellowOnBlack() noexcept;
};

}  // namespace Spark::Gui
