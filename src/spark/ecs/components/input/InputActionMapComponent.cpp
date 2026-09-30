#include "spark/ecs/components/input/InputActionMapComponent.hpp"

#include "spark/input/GamepadInputBindings.hpp"
#include "spark/input/KeyboardInputBindings.hpp"

#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] bool NamesEqual(const char* a, const char* b) noexcept {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    return std::strcmp(a, b) == 0;
}

}  // namespace

InputAction& InputActionMapComponent::EmplaceAction(const char* const actionName, const InputActionType type) {
    if (actionName != nullptr) {
        for (std::size_t i = 0; i < actions.GetSize(); ++i) {
            if (actions[i] != nullptr && NamesEqual(actions[i]->GetName().CStr(), actionName)) {
                return *actions[i];
            }
        }
    }
    auto action = MakeUnique<InputAction>(Utf8String(actionName != nullptr ? actionName : ""), type);
    InputAction& ref = *action;
    actions.PushBack(MoveTemp(action));
    return ref;
}

void InputActionMapComponent::BindButton(
        const char* const actionName,
        const int primaryKey,
        const int secondaryKey) {
    InputAction& action = EmplaceAction(actionName, InputActionType::Button);
    action.SetLegacyKeyboardButton(primaryKey, secondaryKey);
    action.AddBinding(MakeUnique<KeyboardButtonBinding>(primaryKey, secondaryKey));
}

void InputActionMapComponent::BindAxis1D(
        const char* const actionName,
        const int negativeKey,
        const int positiveKey,
        const int secondaryNegativeKey,
        const int secondaryPositiveKey) {
    InputAction& action = EmplaceAction(actionName, InputActionType::Axis1D);
    action.SetLegacyKeyboardAxis1D(negativeKey, positiveKey, secondaryNegativeKey, secondaryPositiveKey);
    action.AddBinding(MakeUnique<KeyboardAxis1DBinding>(
            negativeKey, positiveKey, secondaryNegativeKey, secondaryPositiveKey));
}

void InputActionMapComponent::BindGamepadButton(const char* const actionName, const int gamepadButton) {
    InputAction& action = EmplaceAction(actionName, InputActionType::Button);
    action.AddBinding(MakeUnique<GamepadButtonBinding>(gamepadButton));
}

void InputActionMapComponent::BindGamepadAxis1D(
        const char* const actionName,
        const int gamepadAxis,
        const float axisScale) {
    InputAction& action = EmplaceAction(actionName, InputActionType::Axis1D);
    action.AddBinding(MakeUnique<GamepadAxis1DBinding>(gamepadAxis, axisScale));
}

void InputActionMapComponent::BindAxis1DWithGamepadStick(
        const char* const actionName,
        const int negativeKey,
        const int positiveKey,
        const int gamepadAxis,
        const int secondaryNegativeKey,
        const int secondaryPositiveKey,
        const float gamepadScale) {
    BindAxis1D(actionName, negativeKey, positiveKey, secondaryNegativeKey, secondaryPositiveKey);
    InputAction* action = FindAction(actionName);
    if (action != nullptr) {
        action->AddBinding(MakeUnique<GamepadAxis1DBinding>(gamepadAxis, gamepadScale));
    }
}

void InputActionMapComponent::BindButtonWithGamepad(
        const char* const actionName,
        const int primaryKey,
        const int gamepadButton,
        const int secondaryKey) {
    BindButton(actionName, primaryKey, secondaryKey);
    InputAction* action = FindAction(actionName);
    if (action != nullptr) {
        action->AddBinding(MakeUnique<GamepadButtonBinding>(gamepadButton));
    }
}

const InputAction* InputActionMapComponent::FindAction(const char* const actionName) const noexcept {
    if (actionName == nullptr || actionName[0] == '\0') {
        return nullptr;
    }
    for (std::size_t i = 0; i < actions.GetSize(); ++i) {
        if (actions[i] != nullptr && NamesEqual(actions[i]->GetName().CStr(), actionName)) {
            return actions[i].Get();
        }
    }
    return nullptr;
}

InputAction* InputActionMapComponent::FindAction(const char* const actionName) noexcept {
    return const_cast<InputAction*>(static_cast<const InputActionMapComponent*>(this)->FindAction(actionName));
}

}  // namespace Spark
