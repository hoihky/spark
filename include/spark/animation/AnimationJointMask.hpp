#pragma once

#include "spark/core/Array.hpp"

#include <cstdint>

namespace Spark {

class Skeleton;

/** Per-joint overlay weight in [0, 1] (size = skeleton joint count). */
class AnimationJointMask {
public:
    void Clear() noexcept { weights.Clear(); }

    [[nodiscard]] bool IsEmpty() const noexcept { return weights.GetSize() == 0; }
    [[nodiscard]] const Array<float>& GetWeights() const noexcept { return weights; }

    /** Marks <c>rootJointIndex</c> and all descendants with weight 1. */
    void BuildUpperBodySubtree(const Skeleton& skeleton, std::int32_t rootJointIndex) noexcept;

    /** Finds a spine-like joint and builds upper-body mask; returns false if spine not found. */
    [[nodiscard]] bool TryBuildUpperBodyFromSpineName(const Skeleton& skeleton, const char* spineNameHint) noexcept;

private:
    void MarkSubtree_(const Skeleton& skeleton, std::uint32_t jointIndex) noexcept;

    Array<float> weights{};
};

}  // namespace Spark
