#include "spark/render/gpu/VulkanSpvShaderLoader.hpp"

#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"

#include <cstdio>
#include <stdexcept>
#include <string>

namespace Spark {

Array<char> VulkanSpvShaderLoader::ReadSpvFile(const char* filename) const {
    Utf8String pathUtf(SPARK_SHADER_SPV_DIR);
    pathUtf.AppendUtf8("/");
    pathUtf.AppendUtf8(filename);
    FILE* file = std::fopen(pathUtf.CStr(), "rb");
    if (file == nullptr) {
        throw std::runtime_error(std::string("failed to open shader: ") + pathUtf.CStr());
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        throw std::runtime_error(std::string("failed to seek shader: ") + pathUtf.CStr());
    }
    const long sizeLong = std::ftell(file);
    if (sizeLong < 0) {
        std::fclose(file);
        throw std::runtime_error(std::string("failed to tell shader size: ") + pathUtf.CStr());
    }
    const auto size = static_cast<std::size_t>(sizeLong);
    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        throw std::runtime_error(std::string("failed to rewind shader: ") + pathUtf.CStr());
    }
    Array<char> buffer;
    buffer.Resize(size);
    const std::size_t readCount = std::fread(buffer.GetData(), 1, size, file);
    std::fclose(file);
    if (readCount != size) {
        throw std::runtime_error(std::string("failed to read shader: ") + pathUtf.CStr());
    }
    return buffer;
}

VkShaderModule VulkanSpvShaderLoader::CreateShaderModule(const Array<char>& code) const {
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.GetSize();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const std::uint32_t*>(code.GetData());
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateShaderModule failed");
    }
    return shaderModule;
}

}  // namespace Spark
