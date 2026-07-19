#pragma once

#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"

namespace Spark {

class GameWorld;
class Scene;

/**
 * Dear ImGui docking showcase — tool-style panels built with immediate-mode UI.
 * Requires <c>SPARK_ENABLE_IMGUI</c> and an enabled <c>IImGuiLayer</c> on the engine context.
 */
class ImGuiShowcaseDemo {
public:
    void Enter(IEngineContext& context);
    void Leave(IEngineContext& context) noexcept;

    void Simulate(const FrameTiming& timing, IEngineContext& context);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    void BuildToolUi(const FrameTiming& timing, IEngineContext& context);

    bool showDemoWindow = true;
    bool showMetrics = true;
    float sceneExposure = 1.0F;
    int selectedToolTab = 0;
    FrameTiming lastFrameTiming{};
};

}  // namespace Spark
