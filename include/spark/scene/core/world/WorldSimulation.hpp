#pragma once

#include "spark/ai/AiSubsystem.hpp"
#include "spark/audio/SoundSubsystem.hpp"
#include "spark/scene/core/world/WorldView.hpp"
#include "spark/scene/vfx/VfxSubsystem.hpp"

namespace Spark {

class IEngineContext;
struct FrameTiming;

/** Per-frame simulation: components, VFX, audio, AI. */
class WorldSimulation final : public WorldView {
public:
    using WorldView::WorldView;

    void UpdateComponents(const FrameTiming& timing, IEngineContext& context) {
        world_.UpdateGameObjects(timing, context);
    }

    void ProcessVfx() { world_.GetVfxSubsystem().Process(world_); }

    void ProcessAudio(IEngineContext& context) { SoundSubsystem::ProcessFrame(world_, context); }

    void RunAi(const FrameTiming& timing, IEngineContext& context) {
        AiSubsystem::RunFrame(world_, timing, context);
    }

    void RunDefaultSlice(const FrameTiming& timing, IEngineContext& context) {
        UpdateComponents(timing, context);
        ProcessVfx();
        ProcessAudio(context);
    }
};

}  // namespace Spark
