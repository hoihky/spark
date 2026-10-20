#pragma once

#include <cstdint>

namespace Spark {

/**
 * How gameplay velocity and animation root motion combine on the character motor.
 * Default for action games: <c>GameplayDriven</c> (Souls-like — stick moves body, clips follow speed).
 */
enum class MotionArbitrationMode : std::uint8_t {
    /** Controller / gameplay velocity only; root motion is sampled but not applied. */
    GameplayDriven = 0,
    /** Root motion displaces the body; gameplay velocity suppressed for the frame. */
    RootMotionExclusive = 1,
    /** Gameplay velocity plus root motion delta (use sparingly). */
    RootMotionPlusInput = 2,
    /** No translation from gameplay or root motion (in-place attacks, hit reactions). */
    InPlace = 3,
};

}  // namespace Spark
