#include "spark/render/core/VulkanDeviceContext.hpp"

#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/platform/Window.hpp"

#include <stdexcept>

namespace Spark {

VulkanDeviceContext::VulkanDeviceContext(Window& window) : boundWindow(&window) {
    if constexpr (VulkanRendererGpu::kEnableValidationLayers) {
        if (!VulkanRendererGpu::CheckValidationLayerSupport()) {
            throw std::runtime_error("Vulkan validation layers requested but not available");
        }
    }

    CreateInstance();
    CreateDebugMessenger();
    CreateSurface();
    SelectPhysicalDevice();
    CreateLogicalDevice();
}

VulkanDeviceContext::~VulkanDeviceContext() {
    if (device == VK_NULL_HANDLE) {
        return;
    }

    WaitDeviceIdle();
    DestroySwapchain();

    vkDestroyDevice(device, nullptr);
    device = VK_NULL_HANDLE;

    if constexpr (VulkanRendererGpu::kEnableValidationLayers) {
        VulkanRendererGpu::DestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    surface = VK_NULL_HANDLE;
    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;
}

void VulkanDeviceContext::WaitDeviceIdle() const {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }
}

void VulkanDeviceContext::DestroySwapchain() noexcept {
    if (device != VK_NULL_HANDLE) {
        swapchain.Destroy(device);
    }
}

void VulkanDeviceContext::RecreateSwapchain() {
    if (boundWindow == nullptr || device == VK_NULL_HANDLE) {
        return;
    }

    int width = 0;
    int height = 0;
    boundWindow->GetFramebufferSize(width, height);
    while (width == 0 || height == 0) {
        boundWindow->PollEvents();
        boundWindow->GetFramebufferSize(width, height);
    }

    WaitDeviceIdle();
    DestroySwapchain();
    CreateSwapchainImages();
}

bool VulkanDeviceContext::TryGetDrawableSize(int& outWidth, int& outHeight) const noexcept {
    if (swapchain.khr == VK_NULL_HANDLE || swapchain.extent.width == 0 || swapchain.extent.height == 0) {
        return false;
    }
    outWidth = static_cast<int>(swapchain.extent.width);
    outHeight = static_cast<int>(swapchain.extent.height);
    return true;
}

void VulkanDeviceContext::CreateInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Spark";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Spark";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const Array<const char*> extensions = VulkanRendererGpu::GetRequiredInstanceExtensions(*boundWindow);

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
        createInfo.enabledLayerCount =
                static_cast<std::uint32_t>(sizeof(validationLayerNames) / sizeof(validationLayerNames[0]));
        createInfo.ppEnabledLayerNames = validationLayerNames;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateInstance failed");
    }
}

void VulkanDeviceContext::CreateDebugMessenger() {
    if constexpr (!VulkanRendererGpu::kEnableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = VulkanRendererGpu::DefaultDebugMessengerCallback;
    if (VulkanRendererGpu::CreateDebugUtilsMessenger(instance, &debugCreateInfo, nullptr, &debugMessenger) !=
        VK_SUCCESS) {
        throw std::runtime_error("CreateDebugUtilsMessenger failed");
    }
}

void VulkanDeviceContext::CreateSurface() {
    VkSurfaceKHR surfaceHandle = VK_NULL_HANDLE;
    boundWindow->CreateVulkanSurface(instance, &surfaceHandle);
    surface = surfaceHandle;
}

void VulkanDeviceContext::SelectPhysicalDevice() {
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

    queueFamilies = VulkanRendererGpu::FindQueueFamilies(physicalDevice, surface);
}

void VulkanDeviceContext::CreateLogicalDevice() {
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

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{};
    extendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features = deviceFeatures;
    deviceFeatures2.pNext = &extendedDynamicStateFeatures;

    Array<const char*> deviceExtensions;
    deviceExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef SPARK_PLATFORM_APPLE
    deviceExtensions.PushBack("VK_KHR_portability_subset");
#endif

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &deviceFeatures2;
    deviceCreateInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.GetSize());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.GetData();
    deviceCreateInfo.pEnabledFeatures = nullptr;
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

    vkGetDeviceQueue(device, queueFamilies.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilies.presentFamily, 0, &presentQueue);
}

void VulkanDeviceContext::CreateSwapchainImages() {
    const VulkanRendererGpu::SwapchainSupportDetails swapchainSupport =
            VulkanRendererGpu::QuerySwapchainSupport(physicalDevice, surface);
    const VkSurfaceFormatKHR surfaceFormat = VulkanRendererGpu::ChooseSwapSurfaceFormat(swapchainSupport.formats);
    const VkPresentModeKHR presentMode = VulkanRendererGpu::ChooseSwapPresentMode(swapchainSupport.presentModes);
    const VkExtent2D extent =
            VulkanRendererGpu::ChooseSwapExtent(swapchainSupport.capabilities, boundWindow->Handle());

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

    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain.khr) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateSwapchainKHR failed");
    }

    vkGetSwapchainImagesKHR(device, swapchain.khr, &imageCount, nullptr);
    swapchain.images.Resize(static_cast<std::size_t>(imageCount));
    vkGetSwapchainImagesKHR(device, swapchain.khr, &imageCount, swapchain.images.GetData());

    swapchain.imageFormat = surfaceFormat.format;
    swapchain.extent = extent;

    swapchain.imageViews.Resize(swapchain.images.GetSize());
    for (std::size_t i = 0; i < swapchain.images.GetSize(); ++i) {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchain.images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapchain.imageFormat;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchain.imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateImageView failed");
        }
    }
}

}  // namespace Spark
