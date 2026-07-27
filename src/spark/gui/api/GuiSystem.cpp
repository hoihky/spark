#include "spark/gui/api/GuiSystem.hpp"

#include "spark/gui/api/DearImGuiGuiBackend.hpp"
#include "spark/gui/api/SparkNativeGuiBackend.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark::Gui {

GuiSystem& GuiSystem::Get() noexcept {
    static GuiSystem instance;
    return instance;
}

void GuiSystem::BindImGuiLayer(IImGuiLayer* const layer) noexcept {
    boundImGuiLayer = layer;
    dearImGuiRegistered = false;
    dearImGuiBackend.Reset();
    if (activeKind == GuiBackendKind::DearImGui) {
        EnsureDefaultBackends();
    }
}

void GuiSystem::RegisterSparkNativeBackend() {
    if (sparkNativeRegistered) {
        return;
    }
    sparkNativeBackend = MakeUnique<SparkNativeGuiBackend>();
    sparkNativeRegistered = true;
    if (activeBackend == nullptr) {
        activeBackend = sparkNativeBackend.Get();
        activeKind = GuiBackendKind::SparkNative;
    }
}

void GuiSystem::RegisterDearImGuiBackend() {
    if (dearImGuiRegistered || boundImGuiLayer == nullptr) {
        return;
    }
    dearImGuiBackend = MakeUnique<DearImGuiGuiBackend>(*boundImGuiLayer);
    dearImGuiRegistered = true;
}

void GuiSystem::EnsureDefaultBackends() {
    RegisterSparkNativeBackend();
    if (boundImGuiLayer != nullptr) {
        RegisterDearImGuiBackend();
    }
    if (activeBackend == nullptr) {
        activeBackend = sparkNativeBackend.Get();
    }
    if (activeKind == GuiBackendKind::DearImGui && dearImGuiBackend) {
        activeBackend = dearImGuiBackend.Get();
    } else if (sparkNativeBackend) {
        activeBackend = sparkNativeBackend.Get();
        activeKind = GuiBackendKind::SparkNative;
    }
}

void GuiSystem::SetActiveBackend(const GuiBackendKind kind) noexcept {
    EnsureDefaultBackends();
    activeKind = kind;
    if (kind == GuiBackendKind::DearImGui && dearImGuiBackend) {
        activeBackend = dearImGuiBackend.Get();
        if (boundImGuiLayer != nullptr) {
            boundImGuiLayer->SetEnabled(boundImGuiLayer->IsAvailable());
        }
    } else if (sparkNativeBackend) {
        activeBackend = sparkNativeBackend.Get();
        activeKind = GuiBackendKind::SparkNative;
        if (boundImGuiLayer != nullptr) {
            boundImGuiLayer->SetEnabled(false);
        }
    }
}

GuiBackendKind GuiSystem::GetActiveBackend() const noexcept {
    return activeKind;
}

IGuiBackend* GuiSystem::GetActiveBackendPtr() noexcept {
    return activeBackend;
}

const IGuiBackend* GuiSystem::GetActiveBackendPtr() const noexcept {
    return activeBackend;
}

void GuiSystem::ProcessInput(
        GameWorld& world,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    EnsureDefaultBackends();
    if (activeBackend != nullptr) {
        activeBackend->ProcessInput(world, input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
    }
}

void GuiSystem::ProcessInput(
        Scene& scene,
        IInput& input,
        const int framebufferWidth,
        const int framebufferHeight,
        const float contentScaleX,
        const float contentScaleY) {
    ProcessInput(scene.GetWorld(), input, framebufferWidth, framebufferHeight, contentScaleX, contentScaleY);
}

void GuiSystem::Paint(
        const GameWorld& world,
        SceneRenderParams& params,
        const int framebufferWidth,
        const int framebufferHeight) {
    EnsureDefaultBackends();
    if (activeBackend != nullptr) {
        activeBackend->Paint(world, params, framebufferWidth, framebufferHeight);
    }
}

void GuiSystem::Paint(
        const Scene& scene,
        SceneRenderParams& params,
        const int framebufferWidth,
        const int framebufferHeight) {
    Paint(scene.GetWorld(), params, framebufferWidth, framebufferHeight);
}

void GuiSystem::OnEnginePreRender(Window& window, IInput& input, const float deltaTimeSeconds) {
    EnsureDefaultBackends();
    if (activeBackend != nullptr) {
        activeBackend->OnEnginePreRender(window, input, deltaTimeSeconds);
    }
}

void GuiSystem::OnEnginePostRender() {
    if (activeBackend != nullptr) {
        activeBackend->OnEnginePostRender();
    }
}

void GuiSystem::BeginImmediateFrame(const GuiFrameContext& context) {
    if (activeBackend != nullptr) {
        activeBackend->BeginImmediateFrame(context);
    }
}

void GuiSystem::EndImmediateFrame() {
    if (activeBackend != nullptr) {
        activeBackend->EndImmediateFrame();
    }
}

IGuiFrame& GuiSystem::ImmediateFrame() noexcept {
    EnsureDefaultBackends();
    static struct NullGuiFrame final : IGuiFrame {
        void Text(const char*) override {}
        void TextDisabled(const char*) override {}
        void Separator() override {}
        bool Button(const char*, const char*) override { return false; }
        bool Checkbox(const char*, const char*, bool&) override { return false; }
        bool SliderFloat(const char*, const char*, float&, float, float) override { return false; }
        bool DragFloat3(const char*, const char*, float[3], float) override { return false; }
        bool Combo(const char*, const char*, int&, const char* const[], int) override { return false; }
        bool BeginPanel(const char*, const char*, bool*) override { return false; }
        void EndPanel() override {}
        void SameLine(float, float) override {}
        bool Selectable(const char*, const char*, bool) override { return false; }
        bool MenuItem(const char*, bool*) override { return false; }
        void SetCursorPos(float, float) override {}
    } nullFrame;
    if (activeBackend == nullptr) {
        return nullFrame;
    }
    return activeBackend->GetImmediateFrame();
}

}  // namespace Spark::Gui
