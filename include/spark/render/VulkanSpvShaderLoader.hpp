#pragma once

#include "spark/core/Array.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/** Loads SPIR-V from <c>SPARK_SHADER_SPV_DIR</c> and creates <c>VkShaderModule</c> instances. */
class VulkanSpvShaderLoader {
public:
    void SetDevice(VkDevice inDevice) noexcept { device = inDevice; }

    [[nodiscard]] Array<char> ReadSpvFile(const char* filename) const;

    [[nodiscard]] VkShaderModule CreateShaderModule(const Array<char>& code) const;

private:
    VkDevice device = VK_NULL_HANDLE;
};

}  // namespace Spark
