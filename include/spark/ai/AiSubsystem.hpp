#pragma once

namespace Spark {

class GameWorld;
class IEngineContext;
struct FrameTiming;

/**
 * AI frame pipeline: grid nav, navmesh, perception, then <c>AiAgentComponent</c> ticks.
 * Prefer <c>RunFrame</c> over calling the legacy free function.
 */
class AiSubsystem final {
public:
    static void RunFrame(GameWorld& world, const FrameTiming& timing, IEngineContext& context);

    static void ProcessGridNav2D(GameWorld& world, float deltaTimeSeconds) noexcept;
    static void ProcessNavMesh(GameWorld& world) noexcept;
    static void ProcessPerception(GameWorld& world) noexcept;
};

}  // namespace Spark
