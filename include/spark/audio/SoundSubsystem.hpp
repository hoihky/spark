#pragma once

namespace Spark {

class GameWorld;
class IEngineContext;

/**
 * Audio frame pipeline: listeners, ambient zones, then drains <c>SoundCueComponent</c> queues.
 */
class SoundSubsystem final {
public:
    static void ProcessFrame(GameWorld& world, IEngineContext& context);

    static void UpdateListeners(const GameWorld& world) noexcept;
    static void UpdateAmbient(const GameWorld& world) noexcept;
    static void DrainSoundCues(GameWorld& world, IEngineContext& context);
};

/** Legacy entry point; delegates to <c>SoundSubsystem::ProcessFrame</c>. */
void ProcessSoundCues(GameWorld& world, IEngineContext& context);

}  // namespace Spark
