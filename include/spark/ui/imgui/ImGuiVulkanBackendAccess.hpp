#pragma once

#include "spark/config.hpp"

namespace Spark {

class IImGuiLayer;
class IImGuiVulkanBackend;

#if SPARK_ENABLE_IMGUI
[[nodiscard]] IImGuiVulkanBackend* TryGetImGuiVulkanBackend(IImGuiLayer* layer) noexcept;
#endif

}  // namespace Spark
