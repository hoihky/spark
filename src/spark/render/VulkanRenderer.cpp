#include "../../../include/spark/render/VulkanRenderer.hpp"

#include "spark/config.hpp"
#include "spark/render/SceneGroundExtent.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/media/VideoRecorder.hpp"
#include "../../../include/spark/render/Window.hpp"

#include <GLFW/glfw3.h>

#include <chrono>

#include "spark/render/SceneLightingProfile.hpp"
#include "spark/render/VulkanRendererGpu.hpp"
#include "spark/render/VulkanSceneVertexLayout.hpp"
#include "spark/render/VulkanClusteredLightGpu.hpp"
#include "spark/render/VulkanPunctualShadowGpu.hpp"
#include "spark/render/VulkanSceneUniformGpu.hpp"
#include "spark/render/VulkanSpriteInstanceGpu.hpp"
#include "spark/render/VulkanScreenSpaceEffectsPass.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace Spark {

VulkanRenderer::VulkanRenderer(Window& inAppWindow) : appWindow(inAppWindow) {
    if constexpr (VulkanRendererGpu::kEnableValidationLayers) {
        if (!VulkanRendererGpu::CheckValidationLayerSupport()) {
            throw std::runtime_error("Vulkan validation layers requested but not available");
        }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Spark";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Spark";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const Array<const char*> extensions = VulkanRendererGpu::GetRequiredInstanceExtensions(appWindow);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.GetSize());
    createInfo.ppEnabledExtensionNames = extensions.GetData();
#ifdef SPARK_PLATFORM_APPLE
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    if constexpr (VulkanRendererGpu::kEnableValidationLayers) {
        const char* const validationLayerNames[] = {VulkanRendererGpu::kKhronosValidationLayerName};
        createInfo.enabledLayerCount = static_cast<std::uint32_t>(sizeof(validationLayerNames) / sizeof(validationLayerNames[0]));
        createInfo.ppEnabledLayerNames = validationLayerNames;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateInstance failed");
    }

    if constexpr (VulkanRendererGpu::kEnableValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = VulkanRendererGpu::DefaultDebugMessengerCallback;
        if (VulkanRendererGpu::CreateDebugUtilsMessenger(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("CreateDebugUtilsMessenger failed");
        }
    }

    VkSurfaceKHR surfaceHandle = VK_NULL_HANDLE;
    appWindow.CreateVulkanSurface(instance, &surfaceHandle);
    surface = surfaceHandle;

    std::uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("no Vulkan-capable GPU found");
    }
    Array<VkPhysicalDevice> devices;
    devices.Resize(static_cast<std::size_t>(deviceCount));
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.GetData());
    for (std::size_t di = 0; di < devices.GetSize(); ++di) {
        VkPhysicalDevice dev = devices[di];
        if (VulkanRendererGpu::IsDeviceSuitable(dev, surface)) {
            physicalDevice = dev;
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("no suitable Vulkan physical device");
    }

    const VulkanRendererGpu::QueueFamilyIndices queueFamilies = VulkanRendererGpu::FindQueueFamilies(physicalDevice, surface);

    Array<VkDeviceQueueCreateInfo> queueCreateInfos;
    constexpr float queuePriority = 1.0F;
    if (queueFamilies.graphicsFamily == queueFamilies.presentFamily) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.PushBack(queueCreateInfo);
    } else {
        VkDeviceQueueCreateInfo qGraphics{};
        qGraphics.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qGraphics.queueFamilyIndex = queueFamilies.graphicsFamily;
        qGraphics.queueCount = 1;
        qGraphics.pQueuePriorities = &queuePriority;
        queueCreateInfos.PushBack(qGraphics);
        VkDeviceQueueCreateInfo qPresent{};
        qPresent.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qPresent.queueFamilyIndex = queueFamilies.presentFamily;
        qPresent.queueCount = 1;
        qPresent.pQueuePriorities = &queuePriority;
        queueCreateInfos.PushBack(qPresent);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    Array<const char*> deviceExtensions;
    deviceExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef SPARK_PLATFORM_APPLE
    deviceExtensions.PushBack("VK_KHR_portability_subset");
#endif

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.GetSize());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.GetData();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.GetSize());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.GetData();
    if constexpr (VulkanRendererGpu::kEnableValidationLayers) {
        const char* const validationLayerNames[] = {VulkanRendererGpu::kKhronosValidationLayerName};
        deviceCreateInfo.enabledLayerCount =
                static_cast<std::uint32_t>(sizeof(validationLayerNames) / sizeof(validationLayerNames[0]));
        deviceCreateInfo.ppEnabledLayerNames = validationLayerNames;
    }

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDevice failed");
    }
    shaderLoader_.SetDevice(device);

    vkGetDeviceQueue(device, queueFamilies.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilies.presentFamily, 0, &presentQueue);

    CreateCommandPool();
    CreatePersistentSceneResources();
    RecreateSwapchain();
    CreateSyncObjects();
}

VulkanRenderer::~VulkanRenderer() {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    vkDeviceWaitIdle(device);

    DestroyPersistentSceneResources();

    for (std::size_t i = 0; i < maxFramesInFlight; ++i) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    CleanupSwapchain();

    vkDestroyDevice(device, nullptr);

    if constexpr (VulkanRendererGpu::kEnableValidationLayers) {
        VulkanRendererGpu::DestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void VulkanRenderer::CleanupSwapchain() {
    DestroyRenderFinishedSemaphores();
    screenSpaceEffectsPass_.DestroyPipeline(device);
    screenSpaceEffectsPass_.DestroyFlightTargets(device);
    screenSpaceEffectsPass_.DestroyRenderPass(device);
    hdrTonemapPass_.DestroyTonemapPipeline(device);
    hdrTonemapPass_.DestroyFlightTargets(device);
    hdrTonemapPass_.DestroyRenderPass(device);
    DestroyDepthResources();

    presentationFramebuffers.Destroy(device);

    scenePipeline_.DestroyGraphicsPipeline(device);
    screenUi_.DestroyPipelines(device);
    particlePass_.DestroyGraphicsPipeline(device);
    tilemapPass_.DestroyGraphicsPipeline(device);
    spritePass_.DestroyGraphicsPipeline(device);
    presentRenderPass.Destroy(device);
    presentSwapchain.Destroy(device);
}

void VulkanRenderer::RecreateSwapchain() {
    int width = 0;
    int height = 0;
    appWindow.GetFramebufferSize(width, height);
    while (width == 0 || height == 0) {
        appWindow.PollEvents();
        appWindow.GetFramebufferSize(width, height);
    }
    vkDeviceWaitIdle(device);

    if (!commandBuffers.IsEmpty()) {
        vkFreeCommandBuffers(
                device,
                commandPool,
                static_cast<std::uint32_t>(commandBuffers.GetSize()),
                commandBuffers.GetData());
        commandBuffers.Clear();
    }

    CleanupSwapchain();

    const VulkanRendererGpu::SwapchainSupportDetails swapchainSupport = VulkanRendererGpu::QuerySwapchainSupport(physicalDevice, surface);
    const VkSurfaceFormatKHR surfaceFormat = VulkanRendererGpu::ChooseSwapSurfaceFormat(swapchainSupport.formats);
    const VkPresentModeKHR presentMode = VulkanRendererGpu::ChooseSwapPresentMode(swapchainSupport.presentModes);
    const VkExtent2D extent = VulkanRendererGpu::ChooseSwapExtent(swapchainSupport.capabilities, appWindow.Handle());

    std::uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    const VulkanRendererGpu::QueueFamilyIndices queueFamilies = VulkanRendererGpu::FindQueueFamilies(physicalDevice, surface);
    const std::uint32_t familyIndices[2] = {queueFamilies.graphicsFamily, queueFamilies.presentFamily};
    if (queueFamilies.graphicsFamily != queueFamilies.presentFamily) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = familyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchainCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &presentSwapchain.khr) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateSwapchainKHR failed");
    }

    vkGetSwapchainImagesKHR(device, presentSwapchain.khr, &imageCount, nullptr);
    presentSwapchain.images.Resize(static_cast<std::size_t>(imageCount));
    vkGetSwapchainImagesKHR(device, presentSwapchain.khr, &imageCount, presentSwapchain.images.GetData());

    presentSwapchain.imageFormat = surfaceFormat.format;
    presentSwapchain.extent = extent;
    screenshotCapture_.Create(physicalDevice, device, surfaceFormat.format);
    screenshotCapture_.EnsureBuffer(extent);
    videoCapture_.Create(physicalDevice, device, surfaceFormat.format);
    videoCapture_.EnsureBuffer(extent);

    presentSwapchain.imageViews.Resize(presentSwapchain.images.GetSize());
    for (std::size_t i = 0; i < presentSwapchain.images.GetSize(); ++i) {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = presentSwapchain.images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = presentSwapchain.imageFormat;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &presentSwapchain.imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateImageView failed");
        }
    }

    sceneDepthFormat = VulkanRendererGpu::FindDepthFormat(physicalDevice);
    CreateDepthResources();
    CreateRenderPass();
    screenSpaceEffectsPass_.CreateRenderPass(device);
    RecreateHdrFlightTargets();
    screenSpaceEffectsPass_.RecreateFlightTargets(
            physicalDevice, device, presentSwapchain.extent, sceneDepthFormat, maxFramesInFlight);
    scenePipeline_.CreateGraphicsPipeline(
            device, hdrTonemapPass_.HdrRenderPass(), descriptorSetLayout, shaderLoader_);
    screenUi_.CreatePipelines(device, presentRenderPass.vkPass);
    particlePass_.CreateGraphicsPipeline(device, hdrTonemapPass_.HdrRenderPass());
    tilemapPass_.CreateGraphicsPipeline(
            device, hdrTonemapPass_.HdrRenderPass(), descriptorSetLayout, shaderLoader_);
    spritePass_.CreateGraphicsPipeline(
            device, hdrTonemapPass_.HdrRenderPass(), descriptorSetLayout, shaderLoader_);
    CreateFramebuffers();
    hdrTonemapPass_.CreateTonemapPipeline(
            physicalDevice, device, presentRenderPass.vkPass, maxFramesInFlight, shaderLoader_);
    screenSpaceEffectsPass_.CreatePipeline(physicalDevice, device, maxFramesInFlight, shaderLoader_);
    for (std::uint32_t fi = 0; fi < maxFramesInFlight; ++fi) {
        if (fi < uniformBuffers.GetSize() && hdrTonemapPass_.HasFlight(fi) && fi < sceneDepthResources.GetSize()) {
            screenSpaceEffectsPass_.UpdateDescriptor(
                    device,
                    fi,
                    uniformBuffers[fi],
                    hdrTonemapPass_.Flight(fi),
                    screenSpaceEffectsPass_.Flight(fi));
        }
    }

    commandBuffers.Resize(presentationFramebuffers.buffers.GetSize());
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<std::uint32_t>(commandBuffers.GetSize());
    if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.GetData()) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateCommandBuffers failed");
    }

    imagesInFlight.Resize(presentSwapchain.images.GetSize());
    for (std::size_t ii = 0; ii < imagesInFlight.GetSize(); ++ii) {
        imagesInFlight[ii] = VK_NULL_HANDLE;
    }

    CreateRenderFinishedSemaphores();
}

void VulkanRenderer::CreateRenderPass() {
    hdrTonemapPass_.CreateRenderPass(device, sceneDepthFormat);
    presentRenderPass.Create(device, presentSwapchain.imageFormat, VK_FORMAT_UNDEFINED);
}

void VulkanRenderer::RecreateHdrFlightTargets() {
    Array<VkImageView> depthViews;
    depthViews.Resize(maxFramesInFlight);
    for (std::size_t fi = 0; fi < maxFramesInFlight; ++fi) {
        depthViews[fi] = sceneDepthResources[fi].view;
    }
    hdrTonemapPass_.RecreateFlightTargets(
            physicalDevice,
            device,
            presentSwapchain.extent,
            maxFramesInFlight,
            depthViews.GetData(),
            depthViews.GetSize());
}


void VulkanRenderer::CreateFramebuffers() {
    presentationFramebuffers.Create(
            device,
            presentRenderPass.vkPass,
            presentSwapchain.extent,
            presentSwapchain.imageViews,
            VK_NULL_HANDLE);
}

void VulkanRenderer::CreateCommandPool() {
    const VulkanRendererGpu::QueueFamilyIndices queueFamilies = VulkanRendererGpu::FindQueueFamilies(physicalDevice, surface);
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamily;
    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateCommandPool failed");
    }
}

void VulkanRenderer::RecordSceneCommandBuffer(
        VkCommandBuffer commandBuffer,
        std::uint32_t imageIndex,
        std::uint32_t frameIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    deferredUploadBatch_.Record(commandBuffer, device, sceneTextureUploader_, screenUi_);
    customMeshPool_.RecordUploads(commandBuffer);
    RecordShadowMapPass(commandBuffer, frameIndex);

    hdrTonemapPass_.BeginColorAttachmentBarrierIfNeeded(commandBuffer, frameIndex);

    if (frameIndex < sceneDepthResources.GetSize()) {
        VulkanDepthResources& depth = sceneDepthResources[frameIndex];
        if (depth.image != VK_NULL_HANDLE && depth.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            VkImageMemoryBarrier depthBarrier{};
            depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            depthBarrier.srcAccessMask = 0;
            depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            depthBarrier.image = depth.image;
            depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthBarrier.subresourceRange.levelCount = 1;
            depthBarrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &depthBarrier);
            depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }

    VkClearValue clearColors[2]{};
    clearColors[0].color.float32[0] = 0.0F;
    clearColors[0].color.float32[1] = 0.0F;
    clearColors[0].color.float32[2] = 0.0F;
    clearColors[0].color.float32[3] = 1.0F;
    clearColors[1].depthStencil.depth = 1.0F;
    clearColors[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    if (!hdrTonemapPass_.HasFlight(frameIndex) ||
        hdrTonemapPass_.Flight(frameIndex).framebuffer == VK_NULL_HANDLE) {
        throw std::runtime_error("RecordSceneCommandBuffer: HDR framebuffer missing");
    }

    renderPassBeginInfo.renderPass = hdrTonemapPass_.HdrRenderPass();
    renderPassBeginInfo.framebuffer = hdrTonemapPass_.Flight(frameIndex).framebuffer;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = presentSwapchain.extent;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearColors;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (frameIndex < descriptorSets.GetSize()) {
        const VulkanCustomMeshPool::Bindings customMesh = customMeshPool_.GetBindings();
        const VulkanSceneOpaqueRecordContext opaqueCtx{
                .scene = &pendingScene,
                .sceneParamsValid = sceneParamsValid,
                .frameIndex = frameIndex,
                .extent = presentSwapchain.extent,
                .pipelineLit = scenePipeline_.PipelineLit(),
                .pipelineSky = scenePipeline_.PipelineSky(),
                .pipelineLitTransparent = scenePipeline_.PipelineLitTransparent(),
                .pipelineLayout = scenePipeline_.PipelineLayout(),
                .descriptorSet = descriptorSets[frameIndex],
                .meshBindings =
                        {
                                .staticVertexBuffer = vertexBuffer,
                                .staticIndexBuffer = indexBuffer,
                                .customVertexBuffer = customMesh.vertexBuffer,
                                .customIndexBuffer = customMesh.indexBuffer,
                                .cubeIndexCount = cubeIndexCount,
                                .planeIndexCount = planeIndexCount,
                                .planeFirstIndex = planeFirstIndex,
                                .cubeVertexOffset = cubeVertexOffset,
                                .planeVertexOffset = planeVertexOffset,
                                .customDrawPacked = &customDrawPacked,
                        },
                .skinSsboMapped = &skinSsboMapped,
                .maxSkinJoints = kMaxSkinJoints,
        };
        sceneOpaquePass_.Record(commandBuffer, opaqueCtx);
        sceneOpaquePass_.RecordTransparent(commandBuffer, opaqueCtx, customDrawPackedTransparent);
    }

    if (sceneParamsValid && frameIndex < descriptorSets.GetSize()) {
        const VulkanTilemapRecordContext tilemapCtx{
                .scene = &pendingScene,
                .sceneParamsValid = sceneParamsValid,
                .frameIndex = frameIndex,
                .extent = presentSwapchain.extent,
                .vertexBuffer = vertexBuffer,
                .indexBuffer = indexBuffer,
                .quadFirstIndex = spriteQuadFirstIndex,
                .quadIndexCount = spriteQuadIndexCount,
                .descriptorSet = descriptorSets[frameIndex],
        };
        const VulkanSpriteRecordContext spriteCtx{
                .scene = &pendingScene,
                .sceneParamsValid = sceneParamsValid,
                .frameIndex = frameIndex,
                .extent = presentSwapchain.extent,
                .vertexBuffer = vertexBuffer,
                .indexBuffer = indexBuffer,
                .quadFirstIndex = spriteQuadFirstIndex,
                .quadIndexCount = spriteQuadIndexCount,
                .descriptorSet = descriptorSets[frameIndex],
        };
        composite2DPass_.Record(commandBuffer, tilemapPass_, spritePass_, tilemapCtx, spriteCtx);
    }
    particlePass_.Record(commandBuffer, frameIndex, presentSwapchain.extent, pendingScene, sceneParamsValid);

    vkCmdEndRenderPass(commandBuffer);

    hdrTonemapPass_.TransitionColorToShaderRead(commandBuffer, frameIndex);

    const bool ssaoActive = sceneParamsValid && pendingScene.ssaoEnabled;
    if (ssaoActive && frameIndex < uniformBuffers.GetSize() && frameIndex < descriptorSets.GetSize() &&
        hdrTonemapPass_.HasFlight(frameIndex) && screenSpaceEffectsPass_.HasFlight(frameIndex)) {
        screenSpaceEffectsPass_.UpdateDescriptor(
                device,
                frameIndex,
                uniformBuffers[frameIndex],
                hdrTonemapPass_.Flight(frameIndex),
                screenSpaceEffectsPass_.Flight(frameIndex));
        screenSpaceEffectsPass_.Record(
                commandBuffer,
                frameIndex,
                presentSwapchain.extent,
                sceneDepthResources[frameIndex],
                pendingScene);
        hdrTonemapPass_.UpdateTonemapDescriptor(
                device, frameIndex, screenSpaceEffectsPass_.OutputView(frameIndex));
    } else {
        hdrTonemapPass_.UpdateTonemapDescriptor(device, frameIndex);
    }

    hdrTonemapPass_.RecordTonemap(
            commandBuffer,
            imageIndex,
            frameIndex,
            presentRenderPass.vkPass,
            presentSwapchain.extent,
            presentationFramebuffers,
            resolvedLighting.exposure);
    screenUi_.Record(
            commandBuffer,
            frameIndex,
            presentSwapchain.extent,
            pendingScene,
            sceneParamsValid);

    vkCmdEndRenderPass(commandBuffer);

    if (imageIndex < presentSwapchain.images.GetSize()) {
        screenshotCapture_.RecordCopyFromSwapchain(
                commandBuffer,
                presentSwapchain.images[imageIndex],
                presentSwapchain.extent);
        videoCapture_.RecordCopyFromSwapchain(
                commandBuffer,
                presentSwapchain.images[imageIndex],
                presentSwapchain.extent);
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}

void VulkanRenderer::DestroyRenderFinishedSemaphores() {
    for (std::size_t i = 0; i < renderFinishedSemaphores.GetSize(); ++i) {
        if (renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }
    }
    renderFinishedSemaphores.Clear();
}

void VulkanRenderer::CreateRenderFinishedSemaphores() {
    DestroyRenderFinishedSemaphores();

    const std::size_t imageCount = presentSwapchain.images.GetSize();
    renderFinishedSemaphores.Resize(imageCount);

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (std::size_t i = 0; i < imageCount; ++i) {
        if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("CreateRenderFinishedSemaphores failed");
        }
    }
}

void VulkanRenderer::CreateSyncObjects() {
    imageAvailableSemaphores.Resize(maxFramesInFlight);
    inFlightFences.Resize(maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (std::size_t i = 0; i < maxFramesInFlight; ++i) {
        if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("CreateSyncObjects failed");
        }
    }
}

void VulkanRenderer::WaitAllFramesComplete() const {
    // Caller must have already waited inFlightFences[currentFrame] this frame and not reset it yet.
    for (std::size_t i = 0; i < inFlightFences.GetSize(); ++i) {
        if (i == static_cast<std::size_t>(currentFrame)) {
            continue;
        }
        vkWaitForFences(device, 1, &inFlightFences[i], VK_TRUE, UINT64_MAX);
    }
}

void VulkanRenderer::PresentFrame() {
    DrawFrame();
}

bool VulkanRenderer::TryGetDrawableSize(int& outWidth, int& outHeight) const {
    if (presentSwapchain.khr == VK_NULL_HANDLE || presentSwapchain.extent.width == 0 ||
        presentSwapchain.extent.height == 0) {
        return false;
    }
    outWidth = static_cast<int>(presentSwapchain.extent.width);
    outHeight = static_cast<int>(presentSwapchain.extent.height);
    return true;
}

void VulkanRenderer::NotifySwapchainResize() {
    framebufferResized = true;
}

void VulkanRenderer::DrawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    customMeshPool_.ReleaseRetiredBuffers(submittedFrameCounter_);
    screenUi_.ReleaseRetiredFontAtlases(device, submittedFrameCounter_);

    std::uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(
            device,
            presentSwapchain.khr,
            UINT64_MAX,
            imageAvailableSemaphores[currentFrame],
            VK_NULL_HANDLE,
            &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    if (deferredUploadBatch_.NeedsSceneTextureGpuIdle(sceneTextureUploader_, pendingScene, sceneParamsValid)) {
        WaitAllFramesComplete();
    }

    deferredUploadBatch_.Prepare(
            sceneTextureUploader_,
            screenUi_,
            physicalDevice,
            device,
            pendingScene,
            sceneParamsValid,
            submittedFrameCounter_,
            maxFramesInFlight);

    customMeshPool_.UpdateFromScene(pendingScene, submittedFrameCounter_, maxFramesInFlight);
    customMeshPool_.FillCustomDrawPacked(pendingScene, customDrawPacked, customDrawPackedTransparent);
    ++submittedFrameCounter_;

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    hdrTonemapPass_.UpdateTonemapDescriptor(device, currentFrame);
    WriteUniformBuffer(currentFrame);

    if (vkResetCommandBuffer(commandBuffers[imageIndex], 0) != VK_SUCCESS) {
        throw std::runtime_error("vkResetCommandBuffer failed");
    }
    RecordSceneCommandBuffer(commandBuffers[imageIndex], imageIndex, currentFrame);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("vkQueueSubmit failed");
    }

    if (screenshotCapture_.HasPendingCapture() || videoCapture_.IsRecording()) {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        screenshotCapture_.TrySavePendingPng();
        if (videoCapture_.IsRecording()) {
            double ptsSeconds = 0.0;
            if (recordingWallClockValid_) {
                const auto now = std::chrono::steady_clock::now();
                ptsSeconds = std::chrono::duration<double>(now - recordingWallStart_).count();
            }
            videoCapture_.TryCommitFrameAfterFence(ptsSeconds);
        }
    }

    VkSwapchainKHR swapchains[] = {presentSwapchain.khr};
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        RecreateSwapchain();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void VulkanRenderer::RequestScreenshotSave(const char* pathUtf8) {
    screenshotCapture_.RequestSave(pathUtf8);
}

bool VulkanRenderer::BeginVideoRecording(const VideoRecordingSettings& settings) {
    if (videoCapture_.IsRecording()) {
        return false;
    }
    auto recorder = VideoRecorder::Create();
    if (!recorder) {
        return false;
    }
    const VkExtent2D extent = presentSwapchain.extent;
    if (!videoCapture_.BeginRecording(MoveTemp(recorder), settings, extent)) {
        return false;
    }
    recordingWallStart_ = std::chrono::steady_clock::now();
    recordingWallClockValid_ = true;
    return true;
}

void VulkanRenderer::EndVideoRecording() {
    videoCapture_.EndRecording();
    recordingWallClockValid_ = false;
}

bool VulkanRenderer::IsVideoRecording() const {
    return videoCapture_.IsRecording();
}

VideoRecorder* VulkanRenderer::GetActiveVideoRecorder() {
    return videoCapture_.GetRecorder();
}

void VulkanRenderer::SetSceneRenderParams(const SceneRenderParams& params) {
    pendingScene = params;
    sceneParamsValid = true;
    resolvedLighting = ResolveSceneLightingFromParams(
            pendingScene.lightingProfile,
            pendingScene.exposure,
            pendingScene.shadowCascadeNear,
            pendingScene.shadowCascadeFar,
            pendingScene.shadowDistanceMax,
            pendingScene.shadowFadeStartRatio,
            pendingScene.ambientScale,
            pendingScene.directionalShadowsEnabled,
            pendingScene.shadowsCastByDefault,
            pendingScene.shadowsReceiveByDefault,
            pendingScene.useTimeOfDay,
            pendingScene.timeOfDay);
    if (resolvedLighting.timeOfDayApplied || pendingScene.useTimeOfDay) {
        const SceneLightingProfileSettings preset =
                LightingProfileSettingsFor(pendingScene.lightingProfile);
        const float tod = pendingScene.useTimeOfDay ? pendingScene.timeOfDay : preset.timeOfDay.normalizedTime;
        ApplyTimeOfDayLighting(
                tod,
                pendingScene.lightingProfile,
                resolvedLighting,
                &pendingScene.lightDirectionWorld,
                &pendingScene.lightColor,
                &pendingScene.lightIntensity);
    }
    if (pendingScene.ambientColor.x > 0.001F || pendingScene.ambientColor.y > 0.001F ||
        pendingScene.ambientColor.z > 0.001F) {
        resolvedLighting.ambient.groundColor = pendingScene.ambientColor;
    }
}

void VulkanRenderer::CreateDepthResources() {
    for (std::size_t i = 0; i < sceneDepthResources.GetSize(); ++i) {
        sceneDepthResources[i].Destroy(device);
    }
    sceneDepthResources.Resize(maxFramesInFlight);
    for (std::size_t i = 0; i < maxFramesInFlight; ++i) {
        sceneDepthResources[i].Create(device, physicalDevice, presentSwapchain.extent, sceneDepthFormat);
    }
}

void VulkanRenderer::DestroyDepthResources() {
    for (std::size_t i = 0; i < sceneDepthResources.GetSize(); ++i) {
        sceneDepthResources[i].Destroy(device);
    }
    sceneDepthResources.Clear();
}

void VulkanRenderer::DestroyPersistentSceneResources() {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    directionalShadow_.DestroyGraphicsPipeline(device);
    punctualShadow_.DestroyGraphicsPipeline(device);
    for (std::size_t i = 0; i < uniformBuffers.GetSize(); ++i) {
        if (uniformBuffersMapped[i] != nullptr) {
            vkUnmapMemory(device, uniformBuffersMemory[i]);
            uniformBuffersMapped[i] = nullptr;
        }
        if (uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        }
        if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
    }
    uniformBuffers.Clear();
    uniformBuffersMemory.Clear();
    uniformBuffersMapped.Clear();

    for (std::size_t i = 0; i < skinSsboBuffers.GetSize(); ++i) {
        if (skinSsboMapped[i] != nullptr) {
            vkUnmapMemory(device, skinSsboMemory[i]);
            skinSsboMapped[i] = nullptr;
        }
        if (skinSsboBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, skinSsboBuffers[i], nullptr);
        }
        if (skinSsboMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, skinSsboMemory[i], nullptr);
        }
    }
    skinSsboBuffers.Clear();
    skinSsboMemory.Clear();
    skinSsboMapped.Clear();

    clusteredForwardLights_.DestroyBuffers(device);
    screenshotCapture_.Destroy(device);
    videoCapture_.Destroy(device);

    descriptorSets.Clear();
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }

    sceneTextureUploader_.DestroyResources(device);
    customMeshPool_.DestroyResources(device);
    directionalShadow_.DestroyResources(device);
    punctualShadow_.DestroyResources(device);

    customDrawPacked.Clear();
    customDrawPackedTransparent.Clear();

    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if (indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, indexBufferMemory, nullptr);
        indexBufferMemory = VK_NULL_HANDLE;
    }

    particlePass_.DestroyGraphicsPipeline(device);
    tilemapPass_.DestroyGraphicsPipeline(device);
    spritePass_.DestroyGraphicsPipeline(device);
    screenUi_.DestroyPipelines(device);
    particlePass_.DestroyGpuResources(device);
    spritePass_.DestroyGpuResources(device);
    screenUi_.DestroyResources(device);
}


void VulkanRenderer::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding bindings[10]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[6].binding = 6;
    bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[6].descriptorCount = 1;
    bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[7].binding = 7;
    bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[7].descriptorCount = 1;
    bindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[8].binding = 8;
    bindings[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[8].descriptorCount = 1;
    bindings[8].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[9].binding = 9;
    bindings[9].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[9].descriptorCount = 1;
    bindings[9].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 10;
    layoutInfo.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorSetLayout failed");
    }
}

void VulkanRenderer::CreateUniformBuffers() {
    constexpr VkDeviceSize bufferSize = kSceneUniformGpuBytes;
    uniformBuffers.Resize(maxFramesInFlight);
    uniformBuffersMemory.Resize(maxFramesInFlight);
    uniformBuffersMapped.Resize(maxFramesInFlight);

    for (std::size_t i = 0; i < maxFramesInFlight; ++i) {
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[i],
                uniformBuffersMemory[i]);
        if (vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("vkMapMemory UBO failed");
        }
    }
}

void VulkanRenderer::CreateSkinSsboBuffers() {
    skinSsboBuffers.Resize(maxFramesInFlight);
    skinSsboMemory.Resize(maxFramesInFlight);
    skinSsboMapped.Resize(maxFramesInFlight);
    for (std::size_t i = 0; i < maxFramesInFlight; ++i) {
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kSkinSsboBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                skinSsboBuffers[i],
                skinSsboMemory[i]);
        if (vkMapMemory(device, skinSsboMemory[i], 0, kSkinSsboBytes, 0, &skinSsboMapped[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkMapMemory skin SSBO failed");
        }
    }
}

void VulkanRenderer::CreateDescriptorPoolAndSets() {
    VkDescriptorPoolSize poolSizes[3]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = maxFramesInFlight;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = maxFramesInFlight * 4;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = maxFramesInFlight * 5;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = maxFramesInFlight;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorPool failed");
    }

    Array<VkDescriptorSetLayout> layouts;
    layouts.Resize(maxFramesInFlight);
    for (std::size_t li = 0; li < maxFramesInFlight; ++li) {
        layouts[li] = descriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = maxFramesInFlight;
    allocInfo.pSetLayouts = layouts.GetData();

    descriptorSets.Resize(maxFramesInFlight);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.GetData()) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateDescriptorSets failed");
    }

    constexpr VkDeviceSize bufferSize = kSceneUniformGpuBytes;
    for (std::size_t i = 0; i < maxFramesInFlight; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = sceneTextureUploader_.ArrayView();
        imageInfo.sampler = sceneTextureUploader_.Sampler();

        VkWriteDescriptorSet texWrite{};
        texWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        texWrite.dstSet = descriptorSets[i];
        texWrite.dstBinding = 1;
        texWrite.dstArrayElement = 0;
        texWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texWrite.descriptorCount = 1;
        texWrite.pImageInfo = &imageInfo;

        VkDescriptorBufferInfo skinInfo{};
        skinInfo.buffer = skinSsboBuffers[i];
        skinInfo.offset = 0;
        skinInfo.range = kSkinSsboBytes;

        VkWriteDescriptorSet skinWrite{};
        skinWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skinWrite.dstSet = descriptorSets[i];
        skinWrite.dstBinding = 2;
        skinWrite.dstArrayElement = 0;
        skinWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        skinWrite.descriptorCount = 1;
        skinWrite.pBufferInfo = &skinInfo;

        VkDescriptorBufferInfo lightsInfo{};
        lightsInfo.buffer = clusteredForwardLights_.LightsBuffer(static_cast<std::uint32_t>(i));
        lightsInfo.offset = 0;
        lightsInfo.range = kClusterLightsGpuBytes;

        VkWriteDescriptorSet lightsWrite{};
        lightsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightsWrite.dstSet = descriptorSets[i];
        lightsWrite.dstBinding = 4;
        lightsWrite.dstArrayElement = 0;
        lightsWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        lightsWrite.descriptorCount = 1;
        lightsWrite.pBufferInfo = &lightsInfo;

        VkDescriptorBufferInfo clusterInfo{};
        clusterInfo.buffer = clusteredForwardLights_.ClusterBuffer(static_cast<std::uint32_t>(i));
        clusterInfo.offset = 0;
        clusterInfo.range = kClusterGridGpuBytes;

        VkWriteDescriptorSet clusterWrite{};
        clusterWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        clusterWrite.dstSet = descriptorSets[i];
        clusterWrite.dstBinding = 5;
        clusterWrite.dstArrayElement = 0;
        clusterWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        clusterWrite.descriptorCount = 1;
        clusterWrite.pBufferInfo = &clusterInfo;

        VkDescriptorBufferInfo punctualSsboInfo{};
        punctualSsboInfo.buffer = punctualShadow_.SsboBuffer(static_cast<std::uint32_t>(i));
        punctualSsboInfo.offset = 0;
        punctualSsboInfo.range = kPunctualShadowGpuBytes;

        VkWriteDescriptorSet punctualSsboWrite{};
        punctualSsboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        punctualSsboWrite.dstSet = descriptorSets[i];
        punctualSsboWrite.dstBinding = 6;
        punctualSsboWrite.dstArrayElement = 0;
        punctualSsboWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        punctualSsboWrite.descriptorCount = 1;
        punctualSsboWrite.pBufferInfo = &punctualSsboInfo;

        VkDescriptorBufferInfo spriteInstanceInfo{};
        spriteInstanceInfo.buffer = spritePass_.InstanceBuffer(static_cast<std::uint32_t>(i));
        spriteInstanceInfo.offset = 0;
        spriteInstanceInfo.range = static_cast<VkDeviceSize>(kQuadInstanceSsboBytes);

        VkWriteDescriptorSet spriteInstanceWrite{};
        spriteInstanceWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        spriteInstanceWrite.dstSet = descriptorSets[i];
        spriteInstanceWrite.dstBinding = 9;
        spriteInstanceWrite.dstArrayElement = 0;
        spriteInstanceWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        spriteInstanceWrite.descriptorCount = 1;
        spriteInstanceWrite.pBufferInfo = &spriteInstanceInfo;

        const std::size_t shadowFlight = i;
        const bool hasSunShadow = directionalShadow_.HasFlightDepthView(static_cast<std::uint32_t>(shadowFlight));
        const bool hasPunctualShadow = punctualShadow_.HasFlightResources(static_cast<std::uint32_t>(shadowFlight));

        VkDescriptorImageInfo sunShadowInfo{};
        VkWriteDescriptorSet sunShadowWrite{};
        if (hasSunShadow) {
            sunShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            sunShadowInfo.imageView = directionalShadow_.Flight(static_cast<std::uint32_t>(shadowFlight)).depthView;
            sunShadowInfo.sampler = directionalShadow_.CompareSampler();
            sunShadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            sunShadowWrite.dstSet = descriptorSets[i];
            sunShadowWrite.dstBinding = 3;
            sunShadowWrite.dstArrayElement = 0;
            sunShadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            sunShadowWrite.descriptorCount = 1;
            sunShadowWrite.pImageInfo = &sunShadowInfo;
        }

        VkDescriptorImageInfo spotShadowInfo{};
        VkWriteDescriptorSet spotShadowWrite{};
        VkDescriptorImageInfo pointShadowInfo{};
        VkWriteDescriptorSet pointShadowWrite{};
        if (hasPunctualShadow) {
            const VulkanPunctualShadowPass::FlightTarget& pf =
                    punctualShadow_.Flight(static_cast<std::uint32_t>(shadowFlight));
            spotShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            spotShadowInfo.imageView = pf.spot.depthView;
            spotShadowInfo.sampler = punctualShadow_.CompareSampler();
            spotShadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            spotShadowWrite.dstSet = descriptorSets[i];
            spotShadowWrite.dstBinding = 7;
            spotShadowWrite.dstArrayElement = 0;
            spotShadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            spotShadowWrite.descriptorCount = 1;
            spotShadowWrite.pImageInfo = &spotShadowInfo;

            pointShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            pointShadowInfo.imageView = pf.point.depthArrayView;
            pointShadowInfo.sampler = punctualShadow_.CompareSampler();
            pointShadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            pointShadowWrite.dstSet = descriptorSets[i];
            pointShadowWrite.dstBinding = 8;
            pointShadowWrite.dstArrayElement = 0;
            pointShadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            pointShadowWrite.descriptorCount = 1;
            pointShadowWrite.pImageInfo = &pointShadowInfo;
        }

        if (hasSunShadow && hasPunctualShadow) {
            const VkWriteDescriptorSet writes[] = {descriptorWrite,
                    texWrite,
                    skinWrite,
                    sunShadowWrite,
                    lightsWrite,
                    clusterWrite,
                    punctualSsboWrite,
                    spotShadowWrite,
                    pointShadowWrite,
                    spriteInstanceWrite};
            vkUpdateDescriptorSets(device, 10, writes, 0, nullptr);
        } else if (hasSunShadow) {
            const VkWriteDescriptorSet writes[] = {descriptorWrite,
                    texWrite,
                    skinWrite,
                    sunShadowWrite,
                    lightsWrite,
                    clusterWrite,
                    punctualSsboWrite,
                    spriteInstanceWrite};
            vkUpdateDescriptorSets(device, 8, writes, 0, nullptr);
        } else if (hasPunctualShadow) {
            const VkWriteDescriptorSet writes[] = {descriptorWrite,
                    texWrite,
                    skinWrite,
                    lightsWrite,
                    clusterWrite,
                    punctualSsboWrite,
                    spotShadowWrite,
                    pointShadowWrite,
                    spriteInstanceWrite};
            vkUpdateDescriptorSets(device, 9, writes, 0, nullptr);
        } else {
            const VkWriteDescriptorSet writes[] =
                    {descriptorWrite, texWrite, skinWrite, lightsWrite, clusterWrite, punctualSsboWrite, spriteInstanceWrite};
            vkUpdateDescriptorSets(device, 7, writes, 0, nullptr);
        }
    }
}

void VulkanRenderer::CreateSceneGeometry() {
    Mesh cubeMesh = Mesh::CreateUnitCube();
    Array<float> interleaved;
    interleaved.Reserve((cubeMesh.GetVertices().GetSize() + 8) * VulkanSceneVertexLayout::kFloatsPerVertex);
    for (std::size_t i = 0; i < cubeMesh.GetVertices().GetSize(); ++i) {
        VulkanRendererGpu::AppendRigidMeshVertexToInterleaved(cubeMesh.GetVertices()[i], interleaved);
    }

    const std::uint32_t planeVertexBase = static_cast<std::uint32_t>(cubeMesh.GetVertices().GetSize());
    const float groundExtent = kSceneGroundHalfExtent;
    const float groundUvSpan =
            (2.0F * groundExtent) / (std::max)(kSceneGroundWorldUnitsPerTextureRepeat, 1.0e-3F);
    const Mesh::Vertex planeVerts[4] = {
            {{-groundExtent, 0.0F, -groundExtent}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
            {{groundExtent, 0.0F, -groundExtent}, {0.0F, 1.0F, 0.0F}, {groundUvSpan, 0.0F}},
            {{groundExtent, 0.0F, groundExtent}, {0.0F, 1.0F, 0.0F}, {groundUvSpan, groundUvSpan}},
            {{-groundExtent, 0.0F, groundExtent}, {0.0F, 1.0F, 0.0F}, {0.0F, groundUvSpan}},
    };
    for (const Mesh::Vertex& pv : planeVerts) {
        VulkanRendererGpu::AppendRigidMeshVertexToInterleaved(pv, interleaved);
    }

    const std::uint32_t spriteVertexBase = planeVertexBase + 4;
    const Mesh::Vertex spriteVerts[4] = {
            {{-0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
            {{0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
            {{0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
            {{-0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
    };
    for (const Mesh::Vertex& sv : spriteVerts) {
        VulkanRendererGpu::AppendRigidMeshVertexToInterleaved(sv, interleaved);
    }

    Array<std::uint32_t> indices;
    cubeIndexCount = static_cast<std::uint32_t>(cubeMesh.GetIndices().GetSize());
    for (std::size_t i = 0; i < cubeMesh.GetIndices().GetSize(); ++i) {
        indices.PushBack(cubeMesh.GetIndices()[i]);
    }
    planeFirstIndex = static_cast<std::uint32_t>(indices.GetSize());
    planeIndexCount = 6;
    // Both tris CCW when viewed from +Y so normals (0,1,0) match geometry (second tri was 0,2,3 → -Y).
    indices.PushBack(planeVertexBase);
    indices.PushBack(planeVertexBase + 1);
    indices.PushBack(planeVertexBase + 2);
    indices.PushBack(planeVertexBase);
    indices.PushBack(planeVertexBase + 3);
    indices.PushBack(planeVertexBase + 2);

    spriteQuadFirstIndex = static_cast<std::uint32_t>(indices.GetSize());
    spriteQuadIndexCount = 6;
    indices.PushBack(spriteVertexBase);
    indices.PushBack(spriteVertexBase + 1);
    indices.PushBack(spriteVertexBase + 2);
    indices.PushBack(spriteVertexBase);
    indices.PushBack(spriteVertexBase + 3);
    indices.PushBack(spriteVertexBase + 2);

    cubeVertexOffset = 0;
    // Indices already store absolute vertex indices (planeVertexBase..); vertexOffset must be 0.
    // Adding planeVertexBase here would double-offset and read past the vertex buffer (no ground plane).
    planeVertexOffset = 0;

    const VkDeviceSize vbSize = sizeof(float) * interleaved.GetSize();
    const VkDeviceSize ibSize = sizeof(std::uint32_t) * indices.GetSize();

    VkBuffer stagingVertex = VK_NULL_HANDLE;
    VkDeviceMemory stagingVertexMemory = VK_NULL_HANDLE;
    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            vbSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingVertex,
            stagingVertexMemory);
    void* vtxData = nullptr;
    if (vkMapMemory(device, stagingVertexMemory, 0, vbSize, 0, &vtxData) != VK_SUCCESS) {
        throw std::runtime_error("map staging vertex failed");
    }
    std::memcpy(vtxData, interleaved.GetData(), static_cast<std::size_t>(vbSize));
    vkUnmapMemory(device, stagingVertexMemory);

    VkBuffer stagingIndex = VK_NULL_HANDLE;
    VkDeviceMemory stagingIndexMemory = VK_NULL_HANDLE;
    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            ibSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingIndex,
            stagingIndexMemory);
    void* idxData = nullptr;
    if (vkMapMemory(device, stagingIndexMemory, 0, ibSize, 0, &idxData) != VK_SUCCESS) {
        throw std::runtime_error("map staging index failed");
    }
    std::memcpy(idxData, indices.GetData(), static_cast<std::size_t>(ibSize));
    vkUnmapMemory(device, stagingIndexMemory);

    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            vbSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBuffer,
            vertexBufferMemory);
    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            ibSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            indexBuffer,
            indexBufferMemory);

    VulkanRendererGpu::CopyBuffer(device, commandPool, graphicsQueue, stagingVertex, vertexBuffer, vbSize);
    VulkanRendererGpu::CopyBuffer(device, commandPool, graphicsQueue, stagingIndex, indexBuffer, ibSize);

    vkDestroyBuffer(device, stagingVertex, nullptr);
    vkFreeMemory(device, stagingVertexMemory, nullptr);
    vkDestroyBuffer(device, stagingIndex, nullptr);
    vkFreeMemory(device, stagingIndexMemory, nullptr);
}

void VulkanRenderer::CreatePersistentSceneResources() {
    CreateDescriptorSetLayout();
    sceneTextureUploader_.CreateResources(physicalDevice, device, commandPool, graphicsQueue);
    customMeshPool_.CreateResources(physicalDevice, device);
    directionalShadow_.CreateResources(physicalDevice, device, maxFramesInFlight);
    directionalShadow_.CreateGraphicsPipeline(device, descriptorSetLayout, shaderLoader_);
    punctualShadow_.CreateResources(physicalDevice, device, maxFramesInFlight);
    punctualShadow_.CreateGraphicsPipeline(device, descriptorSetLayout, shaderLoader_);
    CreateUniformBuffers();
    CreateSkinSsboBuffers();
    clusteredForwardLights_.CreateBuffers(physicalDevice, device, maxFramesInFlight);
    spritePass_.CreateGpuResources(physicalDevice, device, maxFramesInFlight);
    CreateDescriptorPoolAndSets();
    CreateSceneGeometry();
    screenUi_.CreateResources(physicalDevice, device, maxFramesInFlight, shaderLoader_);
    particlePass_.CreateGpuResources(physicalDevice, device, maxFramesInFlight, shaderLoader_);
}

void VulkanRenderer::RecordShadowMapPass(VkCommandBuffer commandBuffer, std::uint32_t frameIndex) {
    if (frameIndex >= descriptorSets.GetSize()) {
        return;
    }
    const VulkanCustomMeshPool::Bindings customMesh = customMeshPool_.GetBindings();
    VulkanShadowRecordContext ctx{};
    ctx.scene = &pendingScene;
    ctx.sceneParamsValid = sceneParamsValid;
    ctx.vertexBuffer = vertexBuffer;
    ctx.indexBuffer = indexBuffer;
    ctx.customVertexBuffer = customMesh.vertexBuffer;
    ctx.customIndexBuffer = customMesh.indexBuffer;
    ctx.cubeIndexCount = cubeIndexCount;
    ctx.planeIndexCount = planeIndexCount;
    ctx.planeFirstIndex = planeFirstIndex;
    ctx.cubeVertexOffset = cubeVertexOffset;
    ctx.planeVertexOffset = planeVertexOffset;
    ctx.customDrawPacked = &customDrawPacked;
    ctx.skinSsboMapped = &skinSsboMapped;
    ctx.descriptorSet = descriptorSets[frameIndex];
    ctx.maxSkinJoints = kMaxSkinJoints;
    punctualShadow_.Record(commandBuffer, frameIndex, ctx, punctualShadowFrameState_);
    directionalShadow_.Record(commandBuffer, frameIndex, ctx, shadowFrameState_);
}

void VulkanRenderer::WriteUniformBuffer(std::uint32_t frameIndex) {
    if (!sceneParamsValid || frameIndex >= uniformBuffersMapped.GetSize() ||
        uniformBuffersMapped[frameIndex] == nullptr) {
        return;
    }
    punctualShadow_.PrepareAndUpload(frameIndex, pendingScene, resolvedLighting, punctualShadowFrameState_);
    clusteredForwardLights_.BuildAndUpload(
            frameIndex,
            pendingScene,
            resolvedLighting,
            presentSwapchain.extent);
    sceneUniformWriter_.Write(
            uniformBuffersMapped[frameIndex],
            pendingScene,
            resolvedLighting,
            presentSwapchain.extent,
            directionalShadow_,
            frameIndex,
            shadowFrameState_);
}

}  // namespace Spark
