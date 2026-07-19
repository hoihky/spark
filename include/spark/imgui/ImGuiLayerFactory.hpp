#pragma once

#include "spark/memory/UniquePtr.hpp"

namespace Spark {

class IImGuiLayer;

/** Factory (creational pattern): returns a Vulkan-backed layer or a no-op null layer. */
[[nodiscard]] UniquePtr<IImGuiLayer> CreateImGuiLayer();

}  // namespace Spark
