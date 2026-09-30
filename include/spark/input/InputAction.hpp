#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/input/InputActionSample.hpp"
#include "spark/input/InputActionTypes.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

class IInput;
class IInputBinding;

/** Named semantic action aggregating one or more <c>IInputBinding</c> instances (Composite). */
class InputAction final {
public:
    InputAction(Utf8String actionName, const InputActionType actionType) noexcept
            : name(MoveTemp(actionName))
            , type(actionType) {}

    template<typename BindingT>
    void AddBinding(UniquePtr<BindingT> binding) {
        bindings.PushBack(UniquePtr<IInputBinding>(binding.Release()));
    }

    [[nodiscard]] const Utf8String& GetName() const noexcept { return name; }
    [[nodiscard]] InputActionType GetType() const noexcept { return type; }
    [[nodiscard]] std::size_t GetBindingCount() const noexcept { return bindings.GetSize(); }

    InputActionSample Evaluate(const IInput& input) const;

    /** Legacy keyboard-only fields for scene snapshot v1 restore. */
    void SetLegacyKeyboardButton(const int primaryKey, const int secondaryKey) noexcept;
    void SetLegacyKeyboardAxis1D(
            const int negativeKey,
            const int positiveKey,
            const int secondaryNegativeKey,
            const int secondaryPositiveKey) noexcept;

    [[nodiscard]] int GetLegacyPrimaryKey() const noexcept { return legacyPrimaryKey; }
    [[nodiscard]] int GetLegacySecondaryKey() const noexcept { return legacySecondaryKey; }
    [[nodiscard]] int GetLegacyNegativeKey() const noexcept { return legacyNegativeKey; }
    [[nodiscard]] int GetLegacyPositiveKey() const noexcept { return legacyPositiveKey; }
    [[nodiscard]] int GetLegacySecondaryNegativeKey() const noexcept { return legacySecondaryNegativeKey; }
    [[nodiscard]] int GetLegacySecondaryPositiveKey() const noexcept { return legacySecondaryPositiveKey; }

private:
    Utf8String name{};
    InputActionType type = InputActionType::Button;
    Array<UniquePtr<IInputBinding>> bindings{};
    int legacyPrimaryKey = -1;
    int legacySecondaryKey = -1;
    int legacyNegativeKey = -1;
    int legacyPositiveKey = -1;
    int legacySecondaryNegativeKey = -1;
    int legacySecondaryPositiveKey = -1;
};

}  // namespace Spark
