#include "spark/ui/runtime/UiSystem.hpp"

#include "spark/ui/runtime/DearImguiUiBackend.hpp"
#include "spark/ui/runtime/SparkUiBackend.hpp"

namespace Spark::Ui {

UiSystem& UiSystem::Get() noexcept {
    static UiSystem instance{};
    return instance;
}

UiSystem::~UiSystem() = default;

void UiSystem::BindImGuiLayer(IImGuiLayer* layer) noexcept {
    boundImGuiLayer = layer;
    if (dearImguiBackend != nullptr) {
        dearImguiBackend->BindImGuiLayer(layer);
    }
}

void UiSystem::RegisterSparkNativeBackend() {
    if (sparkNativeRegistered) {
        return;
    }
    sparkNativeBackend = MakeUnique<SparkUiBackend>();
    sparkNativeRegistered = true;
    if (activeBackend == nullptr) {
        activeBackend = sparkNativeBackend.Get();
        activeKind = UiBackendKind::SparkNative;
    }
}

void UiSystem::RegisterDearImguiBackend() {
    if (dearImguiRegistered) {
        return;
    }
    dearImguiBackend = MakeUnique<DearImguiUiBackend>(boundImGuiLayer);
    dearImguiRegistered = true;
}

void UiSystem::SetActiveBackend(const UiBackendKind kind) noexcept {
    EnsureDefaultBackends();
    if (kind == UiBackendKind::DearImGui) {
        if (!dearImguiRegistered) {
            RegisterDearImguiBackend();
        }
        if (dearImguiBackend != nullptr) {
            activeBackend = dearImguiBackend.Get();
            activeKind = UiBackendKind::DearImGui;
            return;
        }
    }
    if (sparkNativeBackend != nullptr) {
        activeBackend = sparkNativeBackend.Get();
        activeKind = UiBackendKind::SparkNative;
    }
}

UiBackendKind UiSystem::GetActiveBackend() const noexcept {
    return activeKind;
}

IUiBackend* UiSystem::GetActiveBackendPtr() noexcept {
    return activeBackend;
}

const IUiBackend* UiSystem::GetActiveBackendPtr() const noexcept {
    return activeBackend;
}

void UiSystem::ProcessInput(
        GameWorld& world,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    EnsureDefaultBackends();
    if (activeBackend != nullptr) {
        activeBackend->ProcessInput(
                world, input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
    }
}

void UiSystem::Paint(
        const GameWorld& world,
        SceneRenderParams& params,
        const int framebufferWidth,
        const int framebufferHeight) {
    EnsureDefaultBackends();
    if (activeBackend != nullptr) {
        activeBackend->Paint(world, params, framebufferWidth, framebufferHeight);
    }
}

void UiSystem::OnEnginePreRender(Window& window, IInput& input, const float deltaTimeSeconds) {
    EnsureDefaultBackends();
    if (activeBackend != nullptr) {
        activeBackend->OnEnginePreRender(window, input, deltaTimeSeconds);
    }
}

void UiSystem::OnEnginePostRender() {
    EnsureDefaultBackends();
    if (activeBackend != nullptr) {
        activeBackend->OnEnginePostRender();
    }
}

UiContext UiSystem::GetContext(const UiFrameContext& frame) noexcept {
    UiContext ctx{};
    ctx.frame = frame;
    if (activeBackend != nullptr) {
        ctx.factory = &activeBackend->GetControlsFactory();
    }
    return ctx;
}

void UiSystem::EnsureDefaultBackends() {
    if (!sparkNativeRegistered) {
        RegisterSparkNativeBackend();
    }
}

}  // namespace Spark::Ui
