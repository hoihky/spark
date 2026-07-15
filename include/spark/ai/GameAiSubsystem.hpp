#pragma once

namespace Spark {

class GameWorld;
class IEngineContext;
struct FrameTiming;

/**
 * Game AI subsystem: runs after game-specific simulation unless you reorder calls.
 * Iterates all <c>AiAgentComponent</c> instances and applies FSM, GOAP, fuzzy advisory, path following, and steering.
 */
void SimulateGameAi(GameWorld& world, const FrameTiming& timing, IEngineContext& context);

}  // namespace Spark
