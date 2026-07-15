#pragma once

#include <cstdint>

namespace Spark {

/**
 * Symbolic world facts packed into a bitmask (up to 64 independent boolean facts).
 * GOAP actions specify masked preconditions and effects.
 */
struct GoapActionSpec {
    std::uint64_t preMask = 0;
    std::uint64_t preValue = 0;
    std::uint64_t effectSetMask = 0;
    std::uint64_t effectClearMask = 0;
    float cost = 1.0F;
    std::uint32_t nameId = 0;
};

}  // namespace Spark
