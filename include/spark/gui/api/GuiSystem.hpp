#pragma once

#include "spark/gui/api/GuiBackendKind.hpp"
#include "spark/gui/api/GuiFrameContext.hpp"
#include "spark/gui/api/IGuiFrame.hpp"

#include "spark/memory/UniquePtr.hpp"

namespace Spark {

class GameWorld;
class IImGuiLayer;
class IInput;
class Scene;
class Window;
struct SceneRenderParams;

namespace Gui {

class IGuiBackend;
class SparkNativeGuiBackend;
class DearImGuiGuiBackend;

/**
 * Facade (Single Responsibility: route GUI work to the active <c>IGuiBackend</c> Strategy).
 * Register backends once, then <c>SetActiveBackend</c> to swap implementations.
 */
class GuiSystem {
public:
    static GuiSystem& Get() noexcept;

    /** Wire Dear ImGui layer when compiled in; safe to call with nullptr. */
    void BindImGuiLayer(IImGuiLayer* layer) noexcept;

    void RegisterSparkNativeBackend();
    void RegisterDearImGuiBackend();

    void SetActiveBackend(GuiBackendKind kind) noexcept;
    [[nodiscard]] GuiBackendKind GetActiveBackend() const noexcept;
    [[nodiscard]] IGuiBackend* GetActiveBackendPtr() noexcept;
    [[nodiscard]] const IGuiBackend* GetActiveBackendPtr() const noexcept;

    void ProcessInput(
            GameWorld& world,
            IInput& input,
            int framebufferWidth,
            int framebufferHeight,
            float contentScaleX = 1.0F,
            float contentScaleY = 1.0F);

    void ProcessInput(
            Scene& scene,
            IInput& input,
            int framebufferWidth,
            int framebufferHeight,
            float contentScaleX = 1.0F,
            float contentScaleY = 1.0F);

    void Paint(
            const GameWorld& world,
            SceneRenderParams& params,
            int framebufferWidth,
            int framebufferHeight);

    void Paint(
            const Scene& scene,
            SceneRenderParams& params,
            int framebufferWidth,
            int framebufferHeight);

    void OnEnginePreRender(Window& window, IInput& input, float deltaTimeSeconds);
    void OnEnginePostRender();

    void BeginImmediateFrame(const GuiFrameContext& context);
    void EndImmediateFrame();

    [[nodiscard]] IGuiFrame& ImmediateFrame() noexcept;

private:
    GuiSystem() = default;
    void EnsureDefaultBackends();

    UniquePtr<SparkNativeGuiBackend> sparkNativeBackend;
    UniquePtr<DearImGuiGuiBackend> dearImGuiBackend;
    IGuiBackend* activeBackend = nullptr;
    GuiBackendKind activeKind = GuiBackendKind::SparkNative;
    IImGuiLayer* boundImGuiLayer = nullptr;
    bool sparkNativeRegistered = false;
    bool dearImGuiRegistered = false;
};

/** Portable controls for the active backend (prefer over direct ImGui / Widget calls in new code). */
[[nodiscard]] inline IGuiFrame& Ui() noexcept {
    return GuiSystem::Get().ImmediateFrame();
}

}  // namespace Gui
}  // namespace Spark
