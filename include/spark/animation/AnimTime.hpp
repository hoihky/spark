#pragma once

#include "spark/animation/AnimLoopMode.hpp"

#include <cmath>

namespace Spark {

/** Advances clip local time and reports whether a non-looping clip has finished. */
inline void AdvanceAnimClipTime(
        float deltaSeconds,
        float speed,
        float duration,
        AnimLoopMode loopMode,
        float& inOutTimeSeconds,
        bool& inOutClipFinished) noexcept {
    if (duration <= 1.0e-4F) {
        inOutClipFinished = loopMode != AnimLoopMode::Loop;
        return;
    }
    inOutTimeSeconds += deltaSeconds * speed;
    if (loopMode == AnimLoopMode::Loop) {
        inOutClipFinished = false;
        inOutTimeSeconds = std::fmod(inOutTimeSeconds, duration);
        if (inOutTimeSeconds < 0.0F) {
            inOutTimeSeconds += duration;
        }
        return;
    }
    if (inOutTimeSeconds >= duration) {
        inOutTimeSeconds = duration;
        inOutClipFinished = true;
        return;
    }
    if (inOutTimeSeconds < 0.0F) {
        inOutTimeSeconds = 0.0F;
    }
    inOutClipFinished = false;
}

/** Clamps raw time to the sampling range implied by loop mode (Loop wraps; Once/Hold clamp). */
inline float EvaluateAnimSampleTime(float timeSeconds, float duration, AnimLoopMode loopMode) noexcept {
    if (duration <= 1.0e-4F) {
        return 0.0F;
    }
    if (loopMode == AnimLoopMode::Loop) {
        float t = timeSeconds;
        if (t < 0.0F) {
            t = 0.0F;
        }
        t = std::fmod(t, duration);
        if (t < 0.0F) {
            t += duration;
        }
        return t;
    }
    if (timeSeconds < 0.0F) {
        return 0.0F;
    }
    if (timeSeconds > duration) {
        return duration;
    }
    return timeSeconds;
}

}  // namespace Spark
