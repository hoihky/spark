#pragma once

namespace Spark::Ui {

class IUiElement;

/**
 * Written each frame by <c>UiInputRouter</c> after layout and pointer routing.
 * Games use this to gate 3D picking and gameplay pointer input.
 */
struct UiPointerState {
    float mouseX = 0.0F;
    float mouseY = 0.0F;
    bool pointerOverUi = false;
    bool consumesGamePointer = false;
    bool scrollWheelConsumed = false;
    const IUiElement* tooltipSource = nullptr;
    bool showTooltip = false;
};

[[nodiscard]] const UiPointerState& GetUiPointerState() noexcept;
void SetUiPointerState(const UiPointerState& state) noexcept;

}  // namespace Spark::Ui
