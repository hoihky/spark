#include "spark/config.hpp"

#if SPARK_ENABLE_IMGUI

#include "spark/engine/IInput.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/imgui/ImGuiVulkanBackend.hpp"
#include "spark/imgui/ImGuiVulkanBackendAccess.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/render/platform/Window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <cstdio>
#include <stdexcept>

namespace Spark {

namespace {

void CheckVkResult(const VkResult err) {
    if (err == VK_SUCCESS) {
        return;
    }
    std::fprintf(stderr, "Spark ImGui Vulkan error: VkResult %d\n", static_cast<int>(err));
    throw std::runtime_error("ImGui Vulkan backend failed");
}

class ImGuiVulkanLayer final : public IImGuiLayer, public IImGuiVulkanBackend {
public:
    [[nodiscard]] bool IsAvailable() const noexcept override { return true; }
    [[nodiscard]] bool IsEnabled() const noexcept override { return enabled; }
    void SetEnabled(const bool on) noexcept override { enabled = on; }

    void BeginFrame(Window& window, IInput&, const ImGuiFrameTiming& timing) override {
        if (!enabled || !glfwReady) {
            return;
        }
        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = timing.deltaTimeSeconds > 0.0F ? timing.deltaTimeSeconds : (1.0F / 60.0F);

        int fbW = 0;
        int fbH = 0;
        window.GetFramebufferSize(fbW, fbH);
        float scaleX = 1.0F;
        float scaleY = 1.0F;
        window.GetContentScale(scaleX, scaleY);
        io.DisplaySize = ImVec2(static_cast<float>(fbW > 0 ? fbW : 1), static_cast<float>(fbH > 0 ? fbH : 1));
        io.DisplayFramebufferScale = ImVec2(scaleX, scaleY);

        ImGui_ImplGlfw_NewFrame();
        if (vulkanReady) {
            ImGui_ImplVulkan_NewFrame();
        }
        ImGui::NewFrame();
    }

    void EndFrame() override {
        if (!enabled || !glfwReady) {
            return;
        }
        ImGui::Render();
    }

    [[nodiscard]] bool WantsCaptureMouse() const noexcept override {
        return enabled && glfwReady && ImGui::GetIO().WantCaptureMouse;
    }

    [[nodiscard]] bool WantsCaptureKeyboard() const noexcept override {
        return enabled && glfwReady && ImGui::GetIO().WantCaptureKeyboard;
    }

    void InitGlfw(Window& window) override {
        if (glfwReady) {
            return;
        }
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 6.0F;
        style.FrameRounding = 4.0F;

        if (!ImGui_ImplGlfw_InitForVulkan(window.Handle(), false)) {
            throw std::runtime_error("ImGui_ImplGlfw_InitForVulkan failed");
        }
        glfwReady = true;
    }

    void InstallPlatformCallbacks(Window& window) override {
        if (!glfwReady || glfwCallbacksInstalled) {
            return;
        }
        ImGui_ImplGlfw_InstallCallbacks(window.Handle());
        glfwCallbacksInstalled = true;
    }

    void InitVulkan(const ImGuiVulkanBackendInfo& info) override {
        backendInfo = info;
        if (!glfwReady) {
            return;
        }
        device = info.device;
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = info.instance;
        initInfo.PhysicalDevice = info.physicalDevice;
        initInfo.Device = info.device;
        initInfo.QueueFamily = info.queueFamily;
        initInfo.Queue = info.queue;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = VK_NULL_HANDLE;
        initInfo.DescriptorPoolSize = 1024;
        initInfo.MinImageCount = info.minImageCount;
        initInfo.ImageCount = info.imageCount;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = CheckVkResult;
        initInfo.MinAllocationSize = 0;
        initInfo.UseDynamicRendering = false;
        initInfo.PipelineInfoMain.RenderPass = info.renderPass;
        initInfo.PipelineInfoMain.Subpass = 0;
        initInfo.PipelineInfoMain.MSAASamples = info.msaaSamples;

        if (!ImGui_ImplVulkan_Init(&initInfo)) {
            throw std::runtime_error("ImGui_ImplVulkan_Init failed");
        }
        vulkanReady = true;
    }

    void InvalidateVulkan() noexcept override {
        if (!vulkanReady) {
            return;
        }
        ImGui_ImplVulkan_Shutdown();
        vulkanReady = false;
    }

    void RecreateVulkan(const ImGuiVulkanBackendInfo& info) override {
        backendInfo = info;
        if (!glfwReady) {
            return;
        }
        if (vulkanReady) {
            InvalidateVulkan();
        }
        InitVulkan(info);
    }

    void Shutdown() noexcept override {
        if (device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);
        }
        InvalidateVulkan();
        if (glfwReady) {
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            glfwReady = false;
            glfwCallbacksInstalled = false;
        }
        device = VK_NULL_HANDLE;
    }

    void RecordDrawData(const VkCommandBuffer commandBuffer) override {
        if (!enabled || !vulkanReady) {
            return;
        }
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData == nullptr) {
            return;
        }
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    }

private:
    bool enabled = false;
    bool glfwReady = false;
    bool glfwCallbacksInstalled = false;
    bool vulkanReady = false;
    VkDevice device = VK_NULL_HANDLE;
    ImGuiVulkanBackendInfo backendInfo{};
};

}  // namespace

UniquePtr<IImGuiLayer> CreateImGuiVulkanLayer() {
    return UniquePtr<IImGuiLayer>(new ImGuiVulkanLayer());
}

IImGuiVulkanBackend* TryGetImGuiVulkanBackend(IImGuiLayer* const layer) noexcept {
    return dynamic_cast<IImGuiVulkanBackend*>(layer);
}

}  // namespace Spark

#endif  // SPARK_ENABLE_IMGUI
