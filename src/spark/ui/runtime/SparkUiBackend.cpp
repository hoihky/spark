#include "spark/ui/runtime/SparkUiBackend.hpp"

#include "spark/ui/core/UiPointerState.hpp"
#include "spark/ui/core/SparkUiRenderer.hpp"
#include "spark/ui/runtime/UiScene.hpp"

namespace Spark::Ui {

SparkUiBackend::SparkUiBackend() = default;

void SparkUiBackend::ProcessInput(
        GameWorld& world,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    inputRouter.Process(world, input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
}

void SparkUiBackend::Paint(
        const GameWorld& world,
        SceneRenderParams& params,
        const int framebufferWidth,
        const int framebufferHeight) {
    SparkUiRenderer renderer(params);
    PaintUiCanvases(world, renderer, framebufferWidth, framebufferHeight);
}

bool SparkUiBackend::WantsCaptureMouse() const noexcept {
    return GetUiPointerState().consumesGamePointer || GetUiPointerState().pointerOverUi;
}

bool SparkUiBackend::WantsCaptureKeyboard() const noexcept {
    return GetUiPointerState().consumesGamePointer;
}

}  // namespace Spark::Ui
