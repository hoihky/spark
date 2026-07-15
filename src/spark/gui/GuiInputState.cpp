#include "spark/gui/GuiInputState.hpp"

#include "spark/gui/Widget.hpp"

namespace Spark::Gui {

namespace {

GuiPointerState gState{};
const Widget* gTooltipHoverSource = nullptr;
int gTooltipHoverFrames = 0;
constexpr int kTooltipDelayFrames = 18;

}  // namespace

const GuiPointerState& GetGuiPointerState() noexcept {
    return gState;
}

void SetGuiPointerState(const GuiPointerState& s) noexcept {
    const float clip = gState.tooltipClipRightPx;
    gState = s;
    if (gState.tooltipClipRightPx < 0.0F && clip > 0.0F) {
        gState.tooltipClipRightPx = clip;
    }
    if (s.tooltipSource != gTooltipHoverSource) {
        gTooltipHoverSource = s.tooltipSource;
        gTooltipHoverFrames = 0;
    } else if (s.tooltipSource != nullptr) {
        ++gTooltipHoverFrames;
    } else {
        gTooltipHoverFrames = 0;
    }
    gState.showTooltip = s.tooltipSource != nullptr && gTooltipHoverFrames >= kTooltipDelayFrames;
}

void SetTooltipClipRightPx(const float rightPx) noexcept {
    gState.tooltipClipRightPx = rightPx;
}

}  // namespace Spark::Gui
