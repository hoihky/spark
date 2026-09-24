#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

#include <cstdint>

namespace Spark {

/** Semantic input action kind (Command pattern: bindings map hardware → actions). */
enum class InputActionType : std::uint8_t {
    /** Single key / button, pressed or held. */
    Button = 0,
    /** 1D axis from a negative key and a positive key (e.g. A/D). */
    Axis1D = 1,
};

/** One named action entry inside an <c>InputActionMapComponent</c>. */
struct InputActionDefinition {
    Utf8String name;
    InputActionType type = InputActionType::Button;
    /** For <c>Button</c>. GLFW key code when using <c>GlfwInput</c>. */
    int primaryKey = -1;
    /** Optional alternate key for <c>Button</c>. */
    int secondaryKey = -1;
    /** For <c>Axis1D</c>: key that drives negative direction. */
    int negativeKey = -1;
    /** For <c>Axis1D</c>: key that drives positive direction. */
    int positiveKey = -1;
    /** Optional alternate axis keys (e.g. arrow keys alongside WASD). */
    int secondaryNegativeKey = -1;
    int secondaryPositiveKey = -1;
};

/** Runtime phase for an action edge (mirrors common input-system semantics). */
enum class InputActionPhase : std::uint8_t {
    Started = 0,
    Performed = 1,
    Canceled = 2,
};

}  // namespace Spark
