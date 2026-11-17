#pragma once

namespace Spark {

class IInput;
class Window;

struct ImGuiFrameTiming {
    float deltaTimeSeconds = 0.0F;
};

/**
 * Facade for Dear ImGui frame lifecycle and input capture (Interface Segregation).
 * Implemented by <c>ImGuiVulkanLayer</c> when <c>SPARK_ENABLE_IMGUI</c> is on, otherwise a null object.
 */
class IImGuiLayer {
public:
    virtual ~IImGuiLayer() = default;

    [[nodiscard]] virtual bool IsAvailable() const noexcept = 0;
    [[nodiscard]] virtual bool IsEnabled() const noexcept = 0;
    virtual void SetEnabled(bool enabled) noexcept = 0;

    /** Install GLFW input callbacks after the engine wires its own hooks (chains user callbacks). */
    virtual void InstallPlatformCallbacks(Window& window) = 0;

    /** Call after game update, before building ImGui UI (issues <c>ImGui::NewFrame</c>). */
    virtual void BeginFrame(Window& window, IInput& input, const ImGuiFrameTiming& timing) = 0;
    /** Call after ImGui UI is built; issues <c>ImGui::Render</c> for the Vulkan backend. */
    virtual void EndFrame() = 0;

    [[nodiscard]] virtual bool WantsCaptureMouse() const noexcept = 0;
    [[nodiscard]] virtual bool WantsCaptureKeyboard() const noexcept = 0;
};

}  // namespace Spark
