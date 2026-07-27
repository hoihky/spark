#pragma once

namespace Spark {

class GameWorld;
class IInput;
struct SceneRenderParams;

/** Retained-mode <c>GuiCanvasComponent</c> input and paint (Spark native implementation detail). */
void ProcessSparkRetainedCanvasesInput(
        GameWorld& world,
        IInput& input,
        int framebufferWidth,
        int framebufferHeight,
        float contentScaleX,
        float contentScaleY);

void PaintSparkRetainedCanvases(
        const GameWorld& world,
        SceneRenderParams& params,
        int framebufferWidth,
        int framebufferHeight);

}  // namespace Spark
