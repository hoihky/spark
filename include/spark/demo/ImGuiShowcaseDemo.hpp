#pragma once

#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class Scene;
class UiCanvasComponent;

/**
 * Dear ImGui docking showcase — tool-style panels via retained <c>ImguiDockWorkspace</c>.
 * Requires <c>SPARK_ENABLE_IMGUI</c> and an enabled <c>IImGuiLayer</c> on the engine context.
 */
class ImGuiShowcaseDemo {
public:
    void Enter(IEngineContext& context);
    void Leave(IEngineContext& context, GameWorld& world) noexcept;

    void Simulate(const FrameTiming& timing, IEngineContext& context);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    void BuildRetainedUi(GameWorld& world);
    void PaintOverlayWindows();

    GameObject* uiRoot = nullptr;
    UiCanvasComponent* uiCanvas = nullptr;
    Ui::ISlider* exposureSlider = nullptr;
    Ui::ILabel* frameLabel = nullptr;

    bool showDemoWindow = true;
    bool showMetrics = true;
    float sceneExposure = 1.0F;
    FrameTiming lastFrameTiming{};
    bool uiBuilt = false;
};

}  // namespace Spark
