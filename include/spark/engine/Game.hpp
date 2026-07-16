#pragma once

#include "spark/audio/SoundSubsystem.hpp"
#include "spark/engine/IGame.hpp"
#include "spark/engine/ISceneProvider.hpp"
#include "spark/scene/Scene.hpp"

namespace Spark {

/**
 * Optional base with empty hooks — override only what you need (Open/Closed for typical games).
 * Owns a Scene (graph + mesh assets) for simulation and rendering.
 */
class Game : public IGame, public ISceneProvider {
public:
    [[nodiscard]] Scene& GetScene() noexcept override { return scene; }
    [[nodiscard]] const Scene& GetScene() const noexcept override { return scene; }

    [[nodiscard]] GameWorld& GetWorld() noexcept { return scene.GetWorld(); }
    [[nodiscard]] const GameWorld& GetWorld() const noexcept { return scene.GetWorld(); }

    void OnAttach(IEngineContext& /*context*/) override {}
    void OnDetach() override {}
    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override {
        GetWorld().UpdateGameObjects(timing, context);
        ProcessSoundCues(GetWorld(), context);
    }
    void OnRender(IRenderFrame& /*frame*/, IEngineContext& /*context*/) override {}

protected:
    Scene scene;
};

}  // namespace Spark
