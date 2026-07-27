#pragma once

#include "spark/gui/api/IGuiBackend.hpp"
#include "spark/gui/api/SparkNativeImmediateGuiFrame.hpp"

namespace Spark::Gui {

class SparkNativeGuiBackend final : public IGuiBackend {
public:
    [[nodiscard]] GuiBackendKind GetKind() const noexcept override { return GuiBackendKind::SparkNative; }

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

    void BeginImmediateFrame(const GuiFrameContext& context) override;
    void EndImmediateFrame() override;

    [[nodiscard]] IGuiFrame& GetImmediateFrame() noexcept override { return immediateFrame; }

    [[nodiscard]] bool WantsCaptureMouse() const noexcept override;
    [[nodiscard]] bool WantsCaptureKeyboard() const noexcept override;

private:
    SparkNativeImmediateGuiFrame immediateFrame;
    bool immediateFrameActive = false;
};

}  // namespace Spark::Gui
