#include "spark/input/InputAction.hpp"

#include "spark/input/IInputBinding.hpp"

namespace Spark {

InputActionSample InputAction::Evaluate(const IInput& input) const {
    InputActionSample sample{};
    sample.Reset();
    for (std::size_t i = 0; i < bindings.GetSize(); ++i) {
        if (bindings[i] != nullptr) {
            bindings[i]->Sample(input, sample);
        }
    }
    if (type == InputActionType::Axis1D) {
        sample.FinalizeAxis();
    }
    return sample;
}

void InputAction::SetLegacyKeyboardButton(const int primaryKey, const int secondaryKey) noexcept {
    legacyPrimaryKey = primaryKey;
    legacySecondaryKey = secondaryKey;
}

void InputAction::SetLegacyKeyboardAxis1D(
        const int negativeKey,
        const int positiveKey,
        const int secondaryNegativeKey,
        const int secondaryPositiveKey) noexcept {
    legacyNegativeKey = negativeKey;
    legacyPositiveKey = positiveKey;
    legacySecondaryNegativeKey = secondaryNegativeKey;
    legacySecondaryPositiveKey = secondaryPositiveKey;
}

}  // namespace Spark
