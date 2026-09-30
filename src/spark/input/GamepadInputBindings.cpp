#include "spark/input/GamepadInputBindings.hpp"

#include "spark/engine/IInput.hpp"

namespace Spark {

void GamepadButtonBinding::Sample(const IInput& input, InputActionSample& sample) const {
    if (!input.IsGamepadPresent()) {
        return;
    }
    const bool held = input.IsGamepadButtonDown(button);
    sample.MergeButtonHeld(held);
    if (input.WasGamepadButtonPressedThisFrame(button)) {
        sample.MergeButtonPressedEdge(true);
    }
}

void GamepadAxis1DBinding::Sample(const IInput& input, InputActionSample& sample) const {
    if (!input.IsGamepadPresent()) {
        return;
    }
    sample.MergeAxisContribution(input.GetGamepadAxis(axis) * axisScale);
}

}  // namespace Spark
