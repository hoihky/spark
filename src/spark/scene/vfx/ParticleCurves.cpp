#include "spark/scene/vfx/ParticleCurves.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

float Clamp01(const float t) noexcept {
    return std::clamp(t, 0.0F, 1.0F);
}

}  // namespace

void ParticleFloatCurve::Clear() noexcept {
    keyframeCount = 0;
}

void ParticleFloatCurve::SetEndpoints(const float startValue, const float endValue) noexcept {
    keyframes[0] = Keyframe{0.0F, startValue};
    keyframes[1] = Keyframe{1.0F, endValue};
    keyframeCount = 2;
}

float ParticleFloatCurve::Evaluate(const float normalizedLife) const noexcept {
    if (keyframeCount == 0) {
        return 0.0F;
    }
    if (keyframeCount == 1) {
        return keyframes[0].value;
    }
    const float t = Clamp01(normalizedLife);
    if (t <= keyframes[0].time) {
        return keyframes[0].value;
    }
    if (t >= keyframes[keyframeCount - 1].time) {
        return keyframes[keyframeCount - 1].value;
    }
    for (std::uint8_t i = 0; i + 1 < keyframeCount; ++i) {
        const Keyframe& a = keyframes[i];
        const Keyframe& b = keyframes[i + 1];
        if (t < a.time || t > b.time) {
            continue;
        }
        const float span = b.time - a.time;
        if (span <= 1.0e-6F) {
            return b.value;
        }
        const float u = (t - a.time) / span;
        return a.value + (b.value - a.value) * u;
    }
    return keyframes[keyframeCount - 1].value;
}

void ParticleColorCurve::Clear() noexcept {
    keyframeCount = 0;
}

void ParticleColorCurve::SetEndpoints(const Vector4& startColor, const Vector4& endColor) noexcept {
    keyframes[0] = Keyframe{0.0F, startColor};
    keyframes[1] = Keyframe{1.0F, endColor};
    keyframeCount = 2;
}

Vector4 ParticleColorCurve::Evaluate(const float normalizedLife) const noexcept {
    if (keyframeCount == 0) {
        return Vector4{1.0F, 1.0F, 1.0F, 1.0F};
    }
    if (keyframeCount == 1) {
        return keyframes[0].color;
    }
    const float t = Clamp01(normalizedLife);
    if (t <= keyframes[0].time) {
        return keyframes[0].color;
    }
    if (t >= keyframes[keyframeCount - 1].time) {
        return keyframes[keyframeCount - 1].color;
    }
    for (std::uint8_t i = 0; i + 1 < keyframeCount; ++i) {
        const Keyframe& a = keyframes[i];
        const Keyframe& b = keyframes[i + 1];
        if (t < a.time || t > b.time) {
            continue;
        }
        const float span = b.time - a.time;
        if (span <= 1.0e-6F) {
            return b.color;
        }
        const float u = (t - a.time) / span;
        return a.color + (b.color - a.color) * u;
    }
    return keyframes[keyframeCount - 1].color;
}

}  // namespace Spark
