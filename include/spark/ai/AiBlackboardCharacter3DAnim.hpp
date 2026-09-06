#pragma once

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Suggested <c>AiBlackboard</c> int slot for <c>Character3DAnimFsmComponent</c> when wired to an
 * <c>AiAgentComponent</c>. Games may use other slots; this is only a convention for interoperability.
 *
 * Values (write from FSM / gameplay, read by the 3D FSM driver):
 * - <c>0</c> — no combat overlay; locomotion clips from speed / input apply.
 * - <c>1</c> — play hurt clip until the non-looping clip ends (driver clears slot to 0).
 * - <c>2</c> — play attack clip until the non-looping clip ends (driver clears slot to 0).
 * - <c>3</c> — play stagger clip until finished.
 * - <c>4</c> — play death clip (holds on last frame when finished).
 */
inline constexpr std::size_t kAiBlackboardIntCharacter3DCombatCommand = 13;

}  // namespace Spark
