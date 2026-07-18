#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/VulkanSceneMeshGpu.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class Mesh;
class SkinnedMesh;

/**
 * Dynamic custom rigid/skinned mesh geometry: CPU pack, async GPU upload via command buffer copies,
 * deferred buffer retirement (no vkDeviceWaitIdle on rebuild).
 */
class VulkanCustomMeshPool {
public:
    struct Bindings {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
    };

    void CreateResources(VkPhysicalDevice physicalDevice, VkDevice device);
    void DestroyResources(VkDevice device);

    /** Call after the in-flight fence for the current frame slot has been waited on. */
    void ReleaseRetiredBuffers(std::uint64_t frameCounter);

    /**
     * Rebuild CPU geometry when the scene fingerprint changes; memcpy into host staging.
     * Device buffer (re)allocation retires the previous set without blocking the GPU.
     */
    void UpdateFromScene(
            const SceneRenderParams& scene,
            std::uint64_t frameCounter,
            std::uint32_t maxFramesInFlight);

    /** Records staging→device copies + vertex/index barriers before custom mesh draws. */
    void RecordUploads(VkCommandBuffer commandBuffer);

    [[nodiscard]] Bindings GetBindings() const noexcept;
    void FillCustomDrawPacked(
            const SceneRenderParams& scene,
            Array<CustomMeshGpuSlice>& outOpaquePacked,
            Array<CustomMeshGpuSlice>& outTransparentPacked) const;

private:
    struct BufferSet {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        VkDeviceSize vertexCapacityBytes = 0;
        VkDeviceSize indexCapacityBytes = 0;
    };

    struct RetiredSet {
        BufferSet buffers{};
        std::uint64_t safeAfterFrame = 0;
    };

    void DestroyBufferSet(BufferSet& set, VkDevice device);
    void QueueRetire(BufferSet&& set, std::uint64_t safeAfterFrame);
    void EnsureStagingCapacity(VkDeviceSize vertexBytes, VkDeviceSize indexBytes);
    bool EnsureDeviceCapacity(
            VkDeviceSize vertexBytes,
            VkDeviceSize indexBytes,
            std::uint64_t frameCounter,
            std::uint32_t maxFramesInFlight);
    [[nodiscard]] std::uint64_t ComputeFingerprint(const SceneRenderParams& scene) const;
    void PackSceneGeometry(const SceneRenderParams& scene, Array<float>& interleaved, Array<std::uint32_t>& indices);

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    BufferSet active{};
    Array<RetiredSet> retired{};

    VkBuffer stagingVertex = VK_NULL_HANDLE;
    VkDeviceMemory stagingVertexMemory = VK_NULL_HANDLE;
    void* stagingVertexMapped = nullptr;
    VkDeviceSize stagingVertexCapacity = 0;

    VkBuffer stagingIndex = VK_NULL_HANDLE;
    VkDeviceMemory stagingIndexMemory = VK_NULL_HANDLE;
    void* stagingIndexMapped = nullptr;
    VkDeviceSize stagingIndexCapacity = 0;

    VkDeviceSize pendingVertexBytes = 0;
    VkDeviceSize pendingIndexBytes = 0;
    bool uploadPending = false;

    std::uint64_t lastFingerprint = 0;
    HashMap<const Mesh*, CustomMeshGpuSlice> rigidSlices{};
    HashMap<const SkinnedMesh*, CustomMeshGpuSlice> skinnedSlices{};
};

}  // namespace Spark
