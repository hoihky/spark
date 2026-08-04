#include "spark/ui/runtime/DearImguiUiBackend.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/ui/core/ImguiUiRenderer.hpp"
#include "spark/ui/core/UiPointerState.hpp"
#include "spark/ui/runtime/UiContextMenu.hpp"
#include "spark/ui/runtime/UiFrameContext.hpp"
#include "spark/ui/runtime/UiScene.hpp"
#include "spark/ui/runtime/UiToolkitSettings.hpp"

namespace Spark::Ui {

DearImguiUiBackend::DearImguiUiBackend(IImGuiLayer* layer) noexcept : imguiLayer(layer) {
    imguiRenderer = MakeUnique<ImguiUiRenderer>(imguiLayer);
}

DearImguiUiBackend::~DearImguiUiBackend() = default;

void DearImguiUiBackend::BindImGuiLayer(IImGuiLayer* layer) noexcept {
    imguiLayer = layer;
    if (imguiRenderer != nullptr) {
        imguiRenderer->SetImGuiLayer(layer);
    }
}

void DearImguiUiBackend::ProcessInput(
        GameWorld& world,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    if (UiToolkitSettings::ShouldProcessSparkUiInput()) {
        inputRouter.Process(world, input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
        return;
    }

    (void)world;
    (void)contentScaleX;
    (void)contentScaleY;

    UiPointerState ptrState{};
    if (framebufferWidth > 0 && framebufferHeight > 0) {
        input.GetCursorFramebufferPixels(ptrState.mouseX, ptrState.mouseY, framebufferWidth, framebufferHeight);
    }
    if (imguiRenderer != nullptr && imguiLayer != nullptr && imguiLayer->IsEnabled()) {
        ptrState.pointerOverUi = imguiRenderer->WantsCaptureMouse();
        ptrState.consumesGamePointer =
                imguiRenderer->WantsCaptureMouse() || imguiRenderer->WantsCaptureKeyboard();
    }
    SetUiPointerState(ptrState);
}

void DearImguiUiBackend::Paint(
        const GameWorld& world,
        SceneRenderParams& /*params*/,
        const int framebufferWidth,
        const int framebufferHeight) {
    if (imguiRenderer == nullptr) {
        return;
    }
    PaintUiCanvases(world, *imguiRenderer, framebufferWidth, framebufferHeight);
    GetUiContextMenu().SetViewportBounds(
            static_cast<float>(framebufferWidth > 0 ? framebufferWidth : 1),
            static_cast<float>(framebufferHeight > 0 ? framebufferHeight : 1));
    GetUiContextMenu().PaintImGui();
}

bool DearImguiUiBackend::WantsCaptureMouse() const noexcept {
    if (imguiRenderer != nullptr && imguiRenderer->WantsCaptureMouse()) {
        return true;
    }
    return GetUiPointerState().consumesGamePointer || GetUiPointerState().pointerOverUi;
}

bool DearImguiUiBackend::WantsCaptureKeyboard() const noexcept {
    if (imguiRenderer != nullptr && imguiRenderer->WantsCaptureKeyboard()) {
        return true;
    }
    return GetUiPointerState().consumesGamePointer;
}

void DearImguiUiBackend::OnEnginePreRender(Window& window, IInput& input, const float deltaTimeSeconds) {
    if (imguiRenderer == nullptr) {
        return;
    }
    UiFrameContext frame{};
    frame.deltaTimeSeconds = deltaTimeSeconds;
    imguiRenderer->BeginBackendFrame(window, input, frame);
}

void DearImguiUiBackend::OnEnginePostRender() {
    if (imguiRenderer != nullptr) {
        imguiRenderer->EndBackendFrame();
    }
}

}  // namespace Spark::Ui
