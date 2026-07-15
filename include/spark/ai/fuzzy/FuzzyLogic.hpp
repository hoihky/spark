#pragma once

#include <cstddef>

namespace Spark {

class AiBlackboard;

/** Triangular membership function on a scalar domain (piecewise linear). */
class FuzzyTriangle final {
public:
    constexpr FuzzyTriangle(const float left, const float peak, const float right) noexcept
            : left(left), peak(peak), right(right) {}

    [[nodiscard]] float Membership(const float x) const noexcept;

private:
    float left = 0.0F;
    float peak = 0.0F;
    float right = 0.0F;
};

/**
 * Tiny fuzzy inference: maps one crisp input (read from a blackboard float slot) through two antecedent
 * triangles, combines with "fuzzy AND" (minimum), then writes a defuzzified scalar to an output slot.
 * (Single Responsibility: one-input-one-output advisory layer.)
 */
class FuzzyAdvisoryModule final {
public:
    FuzzyAdvisoryModule(
            const std::size_t inputSlot,
            const std::size_t outputSlot,
            const FuzzyTriangle& low,
            const FuzzyTriangle& high) noexcept
            : inputSlot(inputSlot), outputSlot(outputSlot), low(low), high(high) {}

    void Evaluate(AiBlackboard& board) const noexcept;

private:
    std::size_t inputSlot = 0;
    std::size_t outputSlot = 0;
    FuzzyTriangle low;
    FuzzyTriangle high;
};

}  // namespace Spark
