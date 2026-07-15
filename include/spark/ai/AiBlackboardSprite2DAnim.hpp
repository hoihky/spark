#pragma once

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Suggested <c>AiBlackboard</c> int slot for <c>Sprite2DCharacterAnimFsmComponent</c> when wired to an
 * <c>AiAgentComponent</c>. Games may use other slots; this is only a convention for interoperability.
 *
 * Values (write from FSM / gameplay, read by the sprite FSM driver):
 * - <c>0</c> — no combat overlay; locomotion clips from velocity apply.
 * - <c>1</c> — play hurt clip until the non-looping clip ends (driver clears slot to 0).
 * - <c>2</c> — play attack clip until the non-looping clip ends (driver clears slot to 0).
 */
inline constexpr std::size_t kAiBlackboardIntSprite2DCombatCommand = 12;

}  // namespace Spark
