#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/** std430 layout for <c>shaders/mesh_instance.glsl</c>. */
class VulkanRigidMeshInstanceGpu {
public:
    float model[16]{};
};

static_assert(sizeof(VulkanRigidMeshInstanceGpu) == 64U);

constexpr std::uint32_t kMaxRigidMeshInstancesGpu = SceneRenderParams::MaxRigidMeshInstances;
constexpr std::size_t kRigidMeshInstanceSsboBytes =
        sizeof(VulkanRigidMeshInstanceGpu) * static_cast<std::size_t>(kMaxRigidMeshInstancesGpu);

}  // namespace Spark