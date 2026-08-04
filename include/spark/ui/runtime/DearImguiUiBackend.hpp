#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/UiInputRouter.hpp"
#include "spark/ui/imgui/DearImguiControlsFactory.hpp"
#include "spark/ui/runtime/IUiBackend.hpp"

namespace Spark {

class IImGuiLayer;

namespace Ui {

class ImguiUiRenderer;

class DearImguiUiBackend final : public IUiBackend {
public:
    explicit DearImguiUiBackend(IImGuiLayer* layer) noexcept;
    ~DearImguiUiBackend() override;

    void BindImGuiLayer(IImGuiLayer* layer) noexcept;

    [[nodiscard]] UiBackendKind GetKind() const noexcept override { return UiBackendKind::DearImGui; }
    [[nodiscard]] IUiControlsFactory& GetControlsFactory() noexcept override { return controlsFactory; }

    void ProcessInput(
            GameWorld& world,
            IInput& input,
            int framebufferWidth,
            int framebufferHeight,
            float contentScaleX,
            float contentScaleY) override;

    void Paint(
            const GameWorld& world,
            SceneRenderParams& params,
            int framebufferWidth,
            int framebufferHeight) override;

    [[nodiscard]] bool WantsCaptureMouse() const noexcept override;
    [[nodiscard]] bool WantsCaptureKeyboard() const noexcept override;

    void OnEnginePreRender(Window& window, IInput& input, float deltaTimeSeconds) override;
    void OnEnginePostRender() override;

private:
    DearImguiControlsFactory controlsFactory{};
    UiInputRouter inputRouter{};
    IImGuiLayer* imguiLayer = nullptr;
    UniquePtr<ImguiUiRenderer> imguiRenderer{};
};

}  // namespace Ui
}  // namespace Spark
