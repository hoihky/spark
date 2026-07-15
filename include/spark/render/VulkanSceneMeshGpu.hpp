#pragma once

#include <cstdint>

namespace Spark {

/** Packed draw slice for custom rigid/skinned meshes in GPU vertex/index buffers. */
struct CustomMeshGpuSlice {
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;
    std::int32_t vertexOffset = 0;
};

}  // namespace Spark
