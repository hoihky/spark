#pragma once

#include "spark/gui/api/GuiBackendKind.hpp"
#include "spark/gui/api/GuiFrameContext.hpp"

namespace Spark {

class GameWorld;
class IInput;
class Window;
struct SceneRenderParams;

namespace Gui {

class IGuiFrame;

/**
 * Strategy: encapsulates a full GUI stack (input routing, paint/recording, portable controls).
 * Swap implementations without changing code that uses <c>IGuiFrame</c>.
 */
class IGuiBackend {
public:
    virtual ~IGuiBackend() = default;

    [[nodiscard]] virtual GuiBackendKind GetKind() const noexcept = 0;

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

    /** Called at start of game <c>OnRender</c> when building portable UI. */
    virtual void BeginImmediateFrame(const GuiFrameContext& context) = 0;
    /** Called after portable UI is built. */
    virtual void EndImmediateFrame() = 0;

    [[nodiscard]] virtual IGuiFrame& GetImmediateFrame() noexcept = 0;

    [[nodiscard]] virtual bool WantsCaptureMouse() const noexcept = 0;
    [[nodiscard]] virtual bool WantsCaptureKeyboard() const noexcept = 0;

    /** Engine hook before game <c>OnRender</c> (Dear ImGui <c>NewFrame</c>). */
    virtual void OnEnginePreRender(Window& /*window*/, IInput& /*input*/, float /*deltaTimeSeconds*/) {}
    /** Engine hook after game <c>OnRender</c> (Dear ImGui <c>Render</c>). */
    virtual void OnEnginePostRender() {}
};

}  // namespace Gui
}  // namespace Spark
