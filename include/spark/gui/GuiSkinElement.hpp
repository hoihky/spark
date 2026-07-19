#pragma once

#include <cstdint>

namespace Spark::Gui {

/**
 * Semantic UI chrome slots. Skins map each element to a <c>GuiSpriteSlice</c> so controls stay
 * asset-pack agnostic (Sprout Lands, Kenney, custom atlases, etc.).
 */
enum class GuiSkinElement : std::uint16_t {
    PanelBackground = 0,
    DialogBackground,
    ButtonNormal,
    ButtonHover,
    ButtonPressed,
    ButtonDisabled,
    CheckboxOff,
    CheckboxOn,
    SliderTrack,
    SliderThumb,
    ProgressBarTrack,
    ProgressBarFill,
    IconPlay,
    IconSettings,
    IconHome,
    Count
};

}  // namespace Spark::Gui
