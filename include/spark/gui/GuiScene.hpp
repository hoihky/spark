#pragma once

#include "spark/gui/GuiInputState.hpp"

namespace Spark {

class GameWorld;
class IInput;
class Scene;
struct SceneRenderParams;

/** Layout + hit-test, then pointer routing to the topmost canvas under the cursor. */
void ProcessGuiCanvasesInput(Scene& scene, IInput& input, int framebufferWidth, int framebufferHeight,
        float contentScaleX = 1.0F, float contentScaleY = 1.0F);
void ProcessGuiCanvasesInput(GameWorld& world, IInput& input, int framebufferWidth, int framebufferHeight,
        float contentScaleX = 1.0F, float contentScaleY = 1.0F);

/** True when the cursor is over GUI that should block gameplay pointer (see <c>GuiInputState</c>). */
[[nodiscard]] inline bool GuiConsumesGamePointer() noexcept {
    return Gui::GetGuiPointerState().consumesGamePointer;
}

/** True when the scroll wheel was consumed by a scrollable widget this frame. */
[[nodiscard]] inline bool GuiScrollWheelConsumed() noexcept {
    return Gui::GetGuiPointerState().scrollWheelConsumed;
}

/** Paints all enabled GUI canvases (low sort order first) into params. Call after clearing draws if needed. */
void PaintGuiCanvases(const Scene& scene, SceneRenderParams& params, int framebufferWidth, int framebufferHeight);
void PaintGuiCanvases(const GameWorld& world, SceneRenderParams& params, int framebufferWidth, int framebufferHeight);

}  // namespace Spark
