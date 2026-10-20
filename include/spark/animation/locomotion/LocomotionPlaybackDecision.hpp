#pragma once

#include "spark/animation/AnimLoopMode.hpp"

#include <cstdint>

namespace Spark {

/** Animator command produced by a locomotion strategy. */
class LocomotionPlaybackDecision final {
public:
    enum class Mode : std::uint8_t {
        Idle = 0,
        SingleClip = 1,
        SpeedBlend1D = 2,
        CardinalDirectionBlend = 3,
        TurnInPlace = 4,
    };

    void SetIdle() noexcept {
        mode = Mode::Idle;
        clipA = 0;
        clipB = 0;
        blend01 = 0.0F;
        loopMode = AnimLoopMode::Loop;
    }

    void SetSingleClip(const std::uint32_t clip, const AnimLoopMode loop) noexcept {
        mode = Mode::SingleClip;
        clipA = clip;
        clipB = clip;
        blend01 = 0.0F;
        loopMode = loop;
    }

    void SetSpeedBlend(const std::uint32_t a, const std::uint32_t b, const float blend) noexcept {
        mode = Mode::SpeedBlend1D;
        clipA = a;
        clipB = b;
        blend01 = blend;
        loopMode = AnimLoopMode::Loop;
    }

    void SetCardinalBlend(const std::uint32_t a, const std::uint32_t b, const float blend) noexcept {
        mode = Mode::CardinalDirectionBlend;
        clipA = a;
        clipB = b;
        blend01 = blend;
        loopMode = AnimLoopMode::Loop;
    }

    void SetTurnInPlace(const std::uint32_t clip) noexcept {
        mode = Mode::TurnInPlace;
        clipA = clip;
        clipB = clip;
        blend01 = 0.0F;
        loopMode = AnimLoopMode::Once;
    }

    [[nodiscard]] Mode GetMode() const noexcept { return mode; }
    [[nodiscard]] std::uint32_t GetClipA() const noexcept { return clipA; }
    [[nodiscard]] std::uint32_t GetClipB() const noexcept { return clipB; }
    [[nodiscard]] float GetBlend01() const noexcept { return blend01; }
    [[nodiscard]] AnimLoopMode GetLoopMode() const noexcept { return loopMode; }

private:
    Mode mode = Mode::Idle;
    std::uint32_t clipA = 0;
    std::uint32_t clipB = 0;
    float blend01 = 0.0F;
    AnimLoopMode loopMode = AnimLoopMode::Loop;
};

}  // namespace Spark
