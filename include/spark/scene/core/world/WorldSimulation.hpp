#pragma once

#include "spark/ai/AiSubsystem.hpp"
#include "spark/audio/SoundSubsystem.hpp"
#include "spark/scene/core/world/WorldView.hpp"
#include "spark/scene/vfx/VfxSubsystem.hpp"

#if SPARK_HAS_GAMEPLAY
#include "spark/gameplay/GameplaySubsystem.hpp"
#endif

namespace Spark {

class IEngineContext;
struct FrameTiming;

/** Per-frame simulation: components, VFX, audio, AI. */
class WorldSimulation final : public WorldView {
public:
    using WorldView::WorldView;

    void UpdateComponents(const FrameTiming& timing, IEngineContext& context) {
        hostWorld.UpdateGameObjects(timing, context);
    }

    void ProcessVfx() { hostWorld.GetVfxSubsystem().Process(hostWorld); }

    void ProcessAudio(IEngineContext& context) { SoundSubsystem::ProcessFrame(hostWorld, context); }

    void RunAi(const FrameTiming& timing, IEngineContext& context) {
        AiSubsystem::RunFrame(hostWorld, timing, context);
    }

    void RunDefaultSlice(const FrameTiming& timing, IEngineContext& context) {
        UpdateComponents(timing, context);
        ProcessVfx();
        ProcessAudio(context);
    }

    /** Flushes pending entity destroys after 2D physics. No-op when <c>SPARK_HAS_GAMEPLAY</c> is 0. Call after <c>Physics().Simulate2D</c>. */
    void RunGameplaySlice() noexcept {
#if SPARK_HAS_GAMEPLAY
        GameplaySubsystem::AfterPhysics2D(hostWorld);
#else
        (void)hostWorld;
#endif
    }
};

}  // namespace Spark
