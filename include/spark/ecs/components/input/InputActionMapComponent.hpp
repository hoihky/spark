#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/input/InputActionTypes.hpp"

namespace Spark {

/**
 * Data-only action map (Flyweight configuration). Bind hardware keys to semantic action names.
 * Consumed by <c>PlayerInputComponent</c> on the same entity.
 */
class InputActionMapComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::InputActionMap;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] const Array<InputActionDefinition>& GetActions() const noexcept { return actions; }
    Array<InputActionDefinition>& GetActions() noexcept { return actions; }

    void ClearActions() noexcept { actions.Clear(); }

    void BindButton(const char* actionName, int primaryKey, int secondaryKey = -1);
    void BindAxis1D(
            const char* actionName,
            int negativeKey,
            int positiveKey,
            int secondaryNegativeKey = -1,
            int secondaryPositiveKey = -1);

    [[nodiscard]] const InputActionDefinition* FindAction(const char* actionName) const noexcept;

private:
    Array<InputActionDefinition> actions{};
};

}  // namespace Spark
