#include "spark/ecs/components/input/InputActionMapComponent.hpp"

#include "spark/core/Utility.hpp"

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

void InputActionMapComponent::BindButton(
        const char* const actionName,
        const int primaryKey,
        const int secondaryKey) {
    InputActionDefinition def{};
    def.name = Utf8String(actionName != nullptr ? actionName : "");
    def.type = InputActionType::Button;
    def.primaryKey = primaryKey;
    def.secondaryKey = secondaryKey;
    actions.PushBack(MoveTemp(def));
}

void InputActionMapComponent::BindAxis1D(
        const char* const actionName,
        const int negativeKey,
        const int positiveKey,
        const int secondaryNegativeKey,
        const int secondaryPositiveKey) {
    InputActionDefinition def{};
    def.name = Utf8String(actionName != nullptr ? actionName : "");
    def.type = InputActionType::Axis1D;
    def.negativeKey = negativeKey;
    def.positiveKey = positiveKey;
    def.secondaryNegativeKey = secondaryNegativeKey;
    def.secondaryPositiveKey = secondaryPositiveKey;
    actions.PushBack(MoveTemp(def));
}

const InputActionDefinition* InputActionMapComponent::FindAction(const char* const actionName) const noexcept {
    if (actionName == nullptr || actionName[0] == '\0') {
        return nullptr;
    }
    for (std::size_t i = 0; i < actions.GetSize(); ++i) {
        if (NamesEqual(actions[i].name.CStr(), actionName)) {
            return &actions[i];
        }
    }
    return nullptr;
}

}  // namespace Spark
