#include "spark/input/KeyboardInputBindings.hpp"

#include "spark/engine/IInput.hpp"

namespace Spark {

namespace {

[[nodiscard]] bool KeyDown(const IInput& input, const int key) noexcept {
    return key >= 0 && input.IsKeyDown(key);
}

[[nodiscard]] bool KeyPressed(const IInput& input, const int key) noexcept {
    return key >= 0 && input.IsKeyPressedThisFrame(key);
}

}  // namespace

void KeyboardButtonBinding::Sample(const IInput& input, InputActionSample& sample) const {
    const bool held = KeyDown(input, primary) || KeyDown(input, secondary);
    sample.MergeButtonHeld(held);
    if (KeyPressed(input, primary) || KeyPressed(input, secondary)) {
        sample.MergeButtonPressedEdge(true);
    }
}

void KeyboardAxis1DBinding::Sample(const IInput& input, InputActionSample& sample) const {
    float axis = 0.0F;
    if (KeyDown(input, negative) || KeyDown(input, secondaryNegative)) {
        axis -= 1.0F;
    }
    if (KeyDown(input, positive) || KeyDown(input, secondaryPositive)) {
        axis += 1.0F;
    }
    sample.MergeAxisContribution(axis);
}

}  // namespace Spark
