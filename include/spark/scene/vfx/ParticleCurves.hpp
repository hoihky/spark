#pragma once

#include "spark/math/Vector4.hpp"

#include <cstdint>

namespace Spark {

/** Normalized-life float curve (0 = spawn, 1 = death). Up to eight sorted keyframes. */
struct ParticleFloatCurve {
    struct Keyframe {
        float time = 0.0F;
        float value = 0.0F;
    };

    static constexpr std::uint8_t kMaxKeyframes = 8;

    Keyframe keyframes[kMaxKeyframes]{};
    std::uint8_t keyframeCount = 0;

    void Clear() noexcept;
    void SetEndpoints(float startValue, float endValue) noexcept;
    [[nodiscard]] bool IsEmpty() const noexcept { return keyframeCount == 0; }
    [[nodiscard]] float Evaluate(float normalizedLife) const noexcept;
};

/** Normalized-life color curve. */
struct ParticleColorCurve {
    struct Keyframe {
        float time = 0.0F;
        Vector4 color{1.0F, 1.0F, 1.0F, 1.0F};
    };

    static constexpr std::uint8_t kMaxKeyframes = 8;

    Keyframe keyframes[kMaxKeyframes]{};
    std::uint8_t keyframeCount = 0;

    void Clear() noexcept;
    void SetEndpoints(const Vector4& startColor, const Vector4& endColor) noexcept;
    [[nodiscard]] bool IsEmpty() const noexcept { return keyframeCount == 0; }
    [[nodiscard]] Vector4 Evaluate(float normalizedLife) const noexcept;
};

}  // namespace Spark
