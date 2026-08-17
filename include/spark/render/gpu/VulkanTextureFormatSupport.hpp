#pragma once

#include "spark/scene/TextureFormat.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/**
 * Selects the scene texture-array encoding for a physical device.
 * Defaults to RGBA8 mipmapped storage for fast runtime uploads (glTF, PNG).
 * Block-compressed arrays are opt-in via SPARK_SCENE_RUNTIME_BLOCK_COMPRESSION.
 */
class SceneTextureArrayFormat {
public:
    explicit SceneTextureArrayFormat(VkPhysicalDevice physicalDevice) noexcept;

    [[nodiscard]] SceneTextureArrayMode SelectMode() const noexcept;

    [[nodiscard]] static bool IsFormatSupported(VkPhysicalDevice physicalDevice, VkFormat format) noexcept;
    [[nodiscard]] static SceneTextureArrayMode SelectSceneArrayMode(VkPhysicalDevice physicalDevice) noexcept;
    [[nodiscard]] static VkFormat ToVkFormat(SceneTextureArrayMode mode) noexcept;
    [[nodiscard]] static TexturePixelFormat ToPixelFormat(SceneTextureArrayMode mode) noexcept;

private:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};

/** @deprecated Use <c>SceneTextureArrayFormat</c>. Kept for existing renderer call sites. */
class VulkanTextureFormatSupport {
public:
    [[nodiscard]] static bool IsFormatSupported(VkPhysicalDevice physicalDevice, VkFormat format) noexcept {
        return SceneTextureArrayFormat::IsFormatSupported(physicalDevice, format);
    }
    [[nodiscard]] static SceneTextureArrayMode SelectSceneArrayMode(VkPhysicalDevice physicalDevice) noexcept {
        return SceneTextureArrayFormat::SelectSceneArrayMode(physicalDevice);
    }
    [[nodiscard]] static VkFormat ToVkFormat(const SceneTextureArrayMode mode) noexcept {
        return SceneTextureArrayFormat::ToVkFormat(mode);
    }
    [[nodiscard]] static TexturePixelFormat ToPixelFormat(const SceneTextureArrayMode mode) noexcept {
        return SceneTextureArrayFormat::ToPixelFormat(mode);
    }
};

}  // namespace Spark
