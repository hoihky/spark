#pragma once

#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiPointerState.hpp"

namespace Spark {

class IInput;
struct SceneRenderParams;

void ProcessUiCanvasesInput(
        GameWorld& world,
        IInput& input,
        int framebufferWidth,
        int framebufferHeight,
        float contentScaleX = 1.0F,
        float contentScaleY = 1.0F);

void ProcessUiCanvasesInput(
        Scene& scene,
        IInput& input,
        int framebufferWidth,
        int framebufferHeight,
        float contentScaleX = 1.0F,
        float contentScaleY = 1.0F);

void PaintUiCanvases(
        const GameWorld& world,
        SceneRenderParams& params,
        int framebufferWidth,
        int framebufferHeight);

void PaintUiCanvases(
        const GameWorld& world,
        Ui::IUiRenderer& renderer,
        int framebufferWidth,
        int framebufferHeight);

void PaintUiCanvases(
        const Scene& scene,
        Ui::IUiRenderer& renderer,
        int framebufferWidth,
        int framebufferHeight);

void PaintUiCanvases(
        const Scene& scene,
        SceneRenderParams& params,
        int framebufferWidth,
        int framebufferHeight);

/** True when the UI router consumed the scroll wheel this frame (gate 3D camera zoom, etc.). */
[[nodiscard]] bool UiScrollWheelConsumed() noexcept;

/** True when pointer is over UI chrome (gate viewport picking / orbit). */
[[nodiscard]] inline bool UiPointerOverUi() noexcept {
    return Ui::GetUiPointerState().pointerOverUi;
}

/** True when UI owns gameplay pointer input this frame (replaces <c>GuiConsumesGamePointer</c>). */
[[nodiscard]] inline bool UiConsumesGamePointer() noexcept {
    return Ui::GetUiPointerState().consumesGamePointer;
}

}  // namespace Spark
