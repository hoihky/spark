#pragma once

#include <cmath>

namespace Spark {

/** Mutable accumulator for one frame of semantic input (Strategy: bindings write, actions read). */
class InputActionSample final {
public:
    void Reset() noexcept {
        buttonHeld = false;
        buttonPressedEdge = false;
        buttonReleasedEdge = false;
        axisValue = 0.0F;
        axisTouched = false;
    }

    void MergeButtonHeld(const bool held) noexcept {
        if (held) {
            buttonHeld = true;
        }
    }

    void MergeButtonPressedEdge(const bool pressed) noexcept {
        if (pressed) {
            buttonPressedEdge = true;
            buttonHeld = true;
        }
    }

    void MergeButtonReleasedEdge(const bool released) noexcept {
        if (released) {
            buttonReleasedEdge = true;
        }
    }

    void MergeAxisContribution(const float contribution) noexcept {
        axisValue += contribution;
        if (contribution != 0.0F) {
            axisTouched = true;
        }
    }

    void FinalizeAxis() noexcept {
        if (axisValue > 1.0F) {
            axisValue = 1.0F;
        } else if (axisValue < -1.0F) {
            axisValue = -1.0F;
        }
        if (axisTouched) {
            buttonHeld = std::fabs(axisValue) > 0.001F;
        }
    }

    [[nodiscard]] bool IsButtonHeld() const noexcept { return buttonHeld; }
    [[nodiscard]] bool WasButtonPressedThisFrame() const noexcept { return buttonPressedEdge; }
    [[nodiscard]] bool WasButtonReleasedThisFrame() const noexcept { return buttonReleasedEdge; }
    [[nodiscard]] float GetAxis1D() const noexcept { return axisValue; }

private:
    bool buttonHeld = false;
    bool buttonPressedEdge = false;
    bool buttonReleasedEdge = false;
    float axisValue = 0.0F;
    bool axisTouched = false;
};

}  // namespace Spark
