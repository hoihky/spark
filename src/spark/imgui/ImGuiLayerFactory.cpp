#include "spark/config.hpp"
#include "spark/imgui/ImGuiLayerFactory.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/render/platform/Window.hpp"

namespace Spark {

#if SPARK_ENABLE_IMGUI
UniquePtr<IImGuiLayer> CreateImGuiVulkanLayer();
#endif

namespace {

class ImGuiNullLayer final : public IImGuiLayer {
public:
    [[nodiscard]] bool IsAvailable() const noexcept override { return false; }
    [[nodiscard]] bool IsEnabled() const noexcept override { return false; }
    void SetEnabled(bool) noexcept override {}
    void InstallPlatformCallbacks(Window&) override {}
    void BeginFrame(Window&, IInput&, const ImGuiFrameTiming&) override {}
    void EndFrame() override {}
    [[nodiscard]] bool WantsCaptureMouse() const noexcept override { return false; }
    [[nodiscard]] bool WantsCaptureKeyboard() const noexcept override { return false; }
};

}  // namespace

UniquePtr<IImGuiLayer> CreateImGuiLayer() {
#if SPARK_ENABLE_IMGUI
    return CreateImGuiVulkanLayer();
#else
    return UniquePtr<IImGuiLayer>(new ImGuiNullLayer());
#endif
}

}  // namespace Spark
