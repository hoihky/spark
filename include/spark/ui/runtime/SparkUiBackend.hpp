#pragma once

#include "spark/ui/runtime/IUiBackend.hpp"
#include "spark/ui/core/UiInputRouter.hpp"
#include "spark/ui/spark/SparkUiControlsFactory.hpp"

namespace Spark::Ui {

class SparkUiBackend final : public IUiBackend {
public:
    SparkUiBackend();

    [[nodiscard]] UiBackendKind GetKind() const noexcept override { return UiBackendKind::SparkNative; }
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

private:
    SparkUiControlsFactory controlsFactory{};
    UiInputRouter inputRouter{};
};

}  // namespace Spark::Ui
