#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"
#include "spark/input/InputAction.hpp"
#include "spark/input/InputActionTypes.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

/**
 * Data-only action map (Flyweight configuration). Bind hardware keys to semantic action names.
 * Consumed by <c>PlayerInputComponent</c> on the same entity.
 */
class InputActionMapComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::InputActionMap;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] const Array<UniquePtr<InputAction>>& GetActions() const noexcept { return actions; }
    Array<UniquePtr<InputAction>>& GetActions() noexcept { return actions; }

    SPARK_SCRIPT_BIND(clear)
    void ClearActions() noexcept { actions.Clear(); }

    SPARK_SCRIPT_BIND(bind_button)
    void BindButton(const char* actionName, int primaryKey, int secondaryKey = -1);

    SPARK_SCRIPT_BIND(bind_axis1d)
    void BindAxis1D(
            const char* actionName,
            int negativeKey,
            int positiveKey,
            int secondaryNegativeKey = -1,
            int secondaryPositiveKey = -1);

    void BindGamepadButton(const char* actionName, int gamepadButton);
    void BindGamepadAxis1D(const char* actionName, int gamepadAxis, float axisScale = 1.0F);

    /** Keyboard + gamepad on the same semantic action (common for MoveX / Jump). */
    void BindAxis1DWithGamepadStick(
            const char* actionName,
            int negativeKey,
            int positiveKey,
            int gamepadAxis,
            int secondaryNegativeKey = -1,
            int secondaryPositiveKey = -1,
            float gamepadScale = 1.0F);

    void BindButtonWithGamepad(
            const char* actionName,
            int primaryKey,
            int gamepadButton,
            int secondaryKey = -1);

    [[nodiscard]] const InputAction* FindAction(const char* actionName) const noexcept;
    [[nodiscard]] InputAction* FindAction(const char* actionName) noexcept;

private:
    InputAction& EmplaceAction(const char* actionName, InputActionType type);

    Array<UniquePtr<InputAction>> actions{};
};

}  // namespace Spark
