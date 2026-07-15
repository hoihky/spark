#pragma once

#include "spark/ai/goap/GoapTypes.hpp"
#include "spark/core/Array.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Goal-oriented planner: forward search in discrete world states until goal constraints match.
 * Single Responsibility: symbolic planning only (no movement).
 */
class GoapPlanner {
public:
    /**
     * Builds a sequence of action indices into <c>actions</c> that transforms <c>startWorld</c> into a state
     * satisfying <c>(world & goalMask) == (goalValue & goalMask)</c>.
     */
    [[nodiscard]] static bool Plan(
            std::uint64_t startWorld,
            std::uint64_t goalMask,
            std::uint64_t goalValue,
            const Array<GoapActionSpec>& actions,
            Array<std::uint32_t>& outPlan);
};

}  // namespace Spark
