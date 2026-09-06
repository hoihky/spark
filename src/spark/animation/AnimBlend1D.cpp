#include "spark/animation/AnimBlend1D.hpp"

namespace Spark {

AnimBlend1DSample EvaluateAnimBlend1D(
        const AnimBlend1DKeyframe* keyframes,
        const std::size_t keyframeCount,
        const float parameter) noexcept {
    AnimBlend1DSample out{};
    if (keyframes == nullptr || keyframeCount == 0) {
        return out;
    }

    if (keyframeCount == 1) {
        out.clipA = keyframes[0].clipIndex;
        out.clipB = keyframes[0].clipIndex;
        out.blend01 = 0.0F;
        out.valid = true;
        return out;
    }

    if (parameter <= keyframes[0].threshold) {
        out.clipA = keyframes[0].clipIndex;
        out.clipB = keyframes[0].clipIndex;
        out.blend01 = 0.0F;
        out.valid = true;
        return out;
    }

    const AnimBlend1DKeyframe& last = keyframes[keyframeCount - 1];
    if (parameter >= last.threshold) {
        out.clipA = last.clipIndex;
        out.clipB = last.clipIndex;
        out.blend01 = 0.0F;
        out.valid = true;
        return out;
    }

    for (std::size_t i = 0; i + 1 < keyframeCount; ++i) {
        const AnimBlend1DKeyframe& a = keyframes[i];
        const AnimBlend1DKeyframe& b = keyframes[i + 1];
        if (parameter < b.threshold) {
            const float span = b.threshold - a.threshold;
            const float t = (span > 1.0e-6F) ? ((parameter - a.threshold) / span) : 0.0F;
            out.clipA = a.clipIndex;
            out.clipB = b.clipIndex;
            out.blend01 = t;
            out.valid = true;
            return out;
        }
    }

    out.clipA = last.clipIndex;
    out.clipB = last.clipIndex;
    out.blend01 = 0.0F;
    out.valid = true;
    return out;
}

}  // namespace Spark
