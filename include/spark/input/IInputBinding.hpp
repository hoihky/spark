#pragma once

#include "spark/input/InputActionSample.hpp"

namespace Spark {

class IInput;

/** Command object: maps one hardware source into a semantic action sample. */
class IInputBinding {
public:
    virtual ~IInputBinding() noexcept = default;
    virtual void Sample(const IInput& input, InputActionSample& sample) const = 0;
};

}  // namespace Spark
