#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/runtime/UiBackendKind.hpp"
#include "spark/ui/runtime/UiContext.hpp"

namespace Spark {

class GameWorld;
class IImGuiLayer;
class IInput;
class Scene;
class Window;
struct SceneRenderParams;

namespace Ui {

class IUiBackend;
class SparkUiBackend;
class DearImguiUiBackend;

/**
 * Facade for the new retained UI stack (<c>spark/ui/</c>).
 * Register backends once, then <c>SetActiveBackend</c> to swap Spark native vs Dear ImGui factories.
 */
class UiSystem {
public:
    static UiSystem& Get() noexcept;

    ~UiSystem();

    void BindImGuiLayer(IImGuiLayer* layer) noexcept;

    void RegisterSparkNativeBackend();
    void RegisterDearImguiBackend();

    void SetActiveBackend(UiBackendKind kind) noexcept;
    [[nodiscard]] UiBackendKind GetActiveBackend() const noexcept;
    [[nodiscard]] IUiBackend* GetActiveBackendPtr() noexcept;
    [[nodiscard]] const IUiBackend* GetActiveBackendPtr() const noexcept;

    void ProcessInput(
            GameWorld& world,
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

    void OnEnginePreRender(Window& window, IInput& input, float deltaTimeSeconds);
    void OnEnginePostRender();

    /** Factory + frame parameters for building retained trees (Phase 1+). */
    [[nodiscard]] UiContext GetContext(const UiFrameContext& frame) noexcept;

private:
    UiSystem() = default;
    void EnsureDefaultBackends();

    UniquePtr<SparkUiBackend> sparkNativeBackend;
    UniquePtr<DearImguiUiBackend> dearImguiBackend;
    IUiBackend* activeBackend = nullptr;
    UiBackendKind activeKind = UiBackendKind::SparkNative;
    IImGuiLayer* boundImGuiLayer = nullptr;
    bool sparkNativeRegistered = false;
    bool dearImguiRegistered = false;
};

}  // namespace Ui
}  // namespace Spark
