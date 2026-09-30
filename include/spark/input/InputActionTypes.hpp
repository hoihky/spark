#pragma once

#include <cstdint>

namespace Spark {

/** Semantic input action kind (Command pattern: bindings map hardware → actions). */
enum class InputActionType : std::uint8_t {
    /** Single key / button, pressed or held. */
    Button = 0,
    /** 1D axis from opposing keys or a stick axis. */
    Axis1D = 1,
};

/** Runtime phase for an action edge (mirrors common input-system semantics). */
enum class InputActionPhase : std::uint8_t {
    Started = 0,
    Performed = 1,
    Canceled = 2,
};

}  // namespace Spark
