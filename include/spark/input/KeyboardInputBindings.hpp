#pragma once

#include "spark/input/IInputBinding.hpp"

namespace Spark {

class KeyboardButtonBinding final : public IInputBinding {
public:
    KeyboardButtonBinding(const int primaryKey, const int secondaryKey = -1) noexcept
            : primary(primaryKey)
            , secondary(secondaryKey) {}

    void Sample(const IInput& input, InputActionSample& sample) const override;

private:
    int primary = -1;
    int secondary = -1;
};

class KeyboardAxis1DBinding final : public IInputBinding {
public:
    KeyboardAxis1DBinding(
            const int negativeKey,
            const int positiveKey,
            const int secondaryNegativeKey = -1,
            const int secondaryPositiveKey = -1) noexcept
            : negative(negativeKey)
            , positive(positiveKey)
            , secondaryNegative(secondaryNegativeKey)
            , secondaryPositive(secondaryPositiveKey) {}

    void Sample(const IInput& input, InputActionSample& sample) const override;

private:
    int negative = -1;
    int positive = -1;
    int secondaryNegative = -1;
    int secondaryPositive = -1;
};

}  // namespace Spark
