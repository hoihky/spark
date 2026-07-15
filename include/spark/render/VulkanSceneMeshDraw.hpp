#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/VulkanSceneMeshGpu.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>

namespace Spark {

/** Which vertex/index buffer pair a scene draw item uses. */
enum class SceneMeshGeometryBinding : std::uint8_t {
    None,
    StaticScene,
    CustomDynamic,
};

/** Resolved index draw range for a single <c>SceneDrawItem</c>. */
struct SceneMeshDrawRange {
    bool drawable = false;
    SceneMeshGeometryBinding binding = SceneMeshGeometryBinding::None;
    std::uint32_t indexCount = 0;
    std::uint32_t firstIndex = 0;
    std::int32_t vertexOffset = 0;
};

/** Non-owning handles for static + custom scene mesh buffers. */
struct SceneMeshDrawBindings {
    VkBuffer staticVertexBuffer = VK_NULL_HANDLE;
    VkBuffer staticIndexBuffer = VK_NULL_HANDLE;
    VkBuffer customVertexBuffer = VK_NULL_HANDLE;
    VkBuffer customIndexBuffer = VK_NULL_HANDLE;
    std::uint32_t cubeIndexCount = 0;
    std::uint32_t planeIndexCount = 0;
    std::uint32_t planeFirstIndex = 0;
    std::int32_t cubeVertexOffset = 0;
    std::int32_t planeVertexOffset = 0;
    const Array<CustomMeshGpuSlice>* customDrawPacked = nullptr;
};

/**
 * Maps a draw item to GPU index range + buffer binding.
 * Shared by the lit opaque pass and directional shadow pass.
 */
[[nodiscard]] SceneMeshDrawRange ResolveSceneMeshDrawRange(
        const SceneDrawItem& draw,
        std::size_t drawIndex,
        const SceneMeshDrawBindings& bindings) noexcept;

}  // namespace Spark
