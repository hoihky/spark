#pragma once

#include "spark/ui/runtime/UiBackendKind.hpp"
#include "spark/ui/runtime/UiFrameContext.hpp"

namespace Spark {

class GameWorld;
class IInput;
class Window;
struct SceneRenderParams;

namespace Ui {

class IUiControlsFactory;

/**
 * Strategy: input routing, paint, and factory access for one UI stack.
 */
class IUiBackend {
public:
    virtual ~IUiBackend() = default;

    [[nodiscard]] virtual UiBackendKind GetKind() const noexcept = 0;
    [[nodiscard]] virtual IUiControlsFactory& GetControlsFactory() noexcept = 0;

    virtual void ProcessInput(
            GameWorld& world,
            IInput& input,
            int framebufferWidth,
            int framebufferHeight,
            float contentScaleX,
            float contentScaleY) = 0;

    virtual void Paint(
            const GameWorld& world,
            SceneRenderParams& params,
            int framebufferWidth,
            int framebufferHeight) = 0;

    [[nodiscard]] virtual bool WantsCaptureMouse() const noexcept = 0;
    [[nodiscard]] virtual bool WantsCaptureKeyboard() const noexcept = 0;

    virtual void OnEnginePreRender(Window& window, IInput& input, float deltaTimeSeconds) {}
    virtual void OnEnginePostRender() {}
};

}  // namespace Ui
}  // namespace Spark
