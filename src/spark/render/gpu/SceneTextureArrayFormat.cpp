#include "spark/render/gpu/VulkanTextureFormatSupport.hpp"

namespace Spark {

SceneTextureArrayFormat::SceneTextureArrayFormat(const VkPhysicalDevice physicalDevice) noexcept
        : physicalDevice(physicalDevice) {}

SceneTextureArrayMode SceneTextureArrayFormat::SelectMode() const noexcept {
    return SelectSceneArrayMode(physicalDevice);
}

bool SceneTextureArrayFormat::IsFormatSupported(
        const VkPhysicalDevice physicalDevice,
        const VkFormat format) noexcept {
    if (physicalDevice == VK_NULL_HANDLE) {
        return false;
    }
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
    return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
}

SceneTextureArrayMode SceneTextureArrayFormat::SelectSceneArrayMode(
        const VkPhysicalDevice physicalDevice) noexcept {
    // Runtime BC7/ASTC encoding during scene upload blocks the render thread for seconds per
    // texture (1024² mips). Use RGBA8 + GPU mip blits for glTF/PNG loads; enable block
    // compression only via SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION for pre-baked KTX2 workflows.
#if defined(SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION) && SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION
#if SPARK_TEXTURE_COMPRESSION
#if defined(SPARK_PLATFORM_APPLE) && SPARK_PLATFORM_APPLE && SPARK_HAS_ASTCENC
    if (IsFormatSupported(physicalDevice, VK_FORMAT_ASTC_4x4_UNORM_BLOCK)) {
        return SceneTextureArrayMode::Astc4x4Mipped;
    }
#endif
#if SPARK_HAS_BC7ENC
    if (IsFormatSupported(physicalDevice, VK_FORMAT_BC7_UNORM_BLOCK)) {
        return SceneTextureArrayMode::Bc7Mipped;
    }
#endif
#if SPARK_HAS_ASTCENC
    if (IsFormatSupported(physicalDevice, VK_FORMAT_ASTC_4x4_UNORM_BLOCK)) {
        return SceneTextureArrayMode::Astc4x4Mipped;
    }
#endif
#else
    (void)physicalDevice;
#endif
#else
    (void)physicalDevice;
#endif
    return SceneTextureArrayMode::Rgba8Mipped;
}

VkFormat SceneTextureArrayFormat::ToVkFormat(const SceneTextureArrayMode mode) noexcept {
    switch (mode) {
        case SceneTextureArrayMode::Bc7Mipped:
            return VK_FORMAT_BC7_UNORM_BLOCK;
        case SceneTextureArrayMode::Astc4x4Mipped:
            return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case SceneTextureArrayMode::Rgba8Mipped:
        default:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

TexturePixelFormat SceneTextureArrayFormat::ToPixelFormat(const SceneTextureArrayMode mode) noexcept {
    switch (mode) {
        case SceneTextureArrayMode::Bc7Mipped:
            return TexturePixelFormat::Bc7Unorm;
        case SceneTextureArrayMode::Astc4x4Mipped:
            return TexturePixelFormat::Astc4x4Unorm;
        case SceneTextureArrayMode::Rgba8Mipped:
        default:
            return TexturePixelFormat::Rgba8Unorm;
    }
}

}  // namespace Spark
