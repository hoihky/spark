#pragma once

#include "spark/input/IInputBinding.hpp"

namespace Spark {

/** GLFW gamepad button index (0 = A on Xbox layout). */
class GamepadButtonBinding final : public IInputBinding {
public:
    explicit GamepadButtonBinding(const int gamepadButton) noexcept : button(gamepadButton) {}

    void Sample(const IInput& input, InputActionSample& sample) const override;

private:
    int button = -1;
};

/** GLFW gamepad axis index with optional invert. */
class GamepadAxis1DBinding final : public IInputBinding {
public:
    GamepadAxis1DBinding(const int gamepadAxis, const float scale = 1.0F) noexcept
            : axis(gamepadAxis)
            , axisScale(scale) {}

    void Sample(const IInput& input, InputActionSample& sample) const override;

private:
    int axis = -1;
    float axisScale = 1.0F;
};

}  // namespace Spark
