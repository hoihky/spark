#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/api/SparkNativeGuiBackend.hpp"

#include "spark/gui/GuiInputState.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/internal/SparkNativeRetainedGuiBridge.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark::Gui {

namespace {

GuiLayoutMetrics BuildImmediateDemoMetrics(const float boost) {
    GuiLayoutMetrics metrics = GuiLayoutMetrics::Default();
    if (boost > 1.0F) {
        metrics.fontBody = 24.0F;
        metrics.fontControl = 28.0F;
        metrics.fontLabel = 30.0F;
        metrics.fontSmall = 18.0F;
        metrics.formRowHeight = 42.0F;
        metrics.listRowHeight = 44.0F;
        metrics.treeRowHeight = 40.0F;
        metrics.panelPadding = 12.0F;
        metrics.groupPadding = 14.0F;
        metrics.controlGap = 6.0F;
    }
    metrics.uiScale = GetEffectiveGuiUiScale() * boost;
    return metrics;
}

}  // namespace

void SparkNativeGuiBackend::ProcessInput(
        GameWorld& world,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    ProcessSparkRetainedCanvasesInput(
            world, input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
}

void SparkNativeGuiBackend::Paint(
        const GameWorld& world,
        SceneRenderParams& params,
        const int framebufferWidth,
        const int framebufferHeight) {
    PaintSparkRetainedCanvases(world, params, framebufferWidth, framebufferHeight);
}

void SparkNativeGuiBackend::BeginImmediateFrame(const GuiFrameContext& context) {
    const float boost = context.immediateUiScale > 0.0F ? context.immediateUiScale : 1.0F;
    SetActiveGuiLayoutMetrics(BuildImmediateDemoMetrics(boost));

    immediateFrameActive = true;
    immediateFrame.BindContext(&context);
    immediateFrame.ResetForFrame();
}

void SparkNativeGuiBackend::EndImmediateFrame() {
    immediateFrame.BindContext(nullptr);
    immediateFrameActive = false;
}

bool SparkNativeGuiBackend::WantsCaptureMouse() const noexcept {
    return GetGuiPointerState().consumesGamePointer || GetGuiPointerState().pointerOverGui;
}

bool SparkNativeGuiBackend::WantsCaptureKeyboard() const noexcept {
    return GetGuiPointerState().consumesGamePointer;
}

}  // namespace Spark::Gui
