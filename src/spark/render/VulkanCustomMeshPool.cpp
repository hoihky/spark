#include "spark/render/VulkanCustomMeshPool.hpp"

#include "spark/core/ContentFingerprint.hpp"
#include "spark/render/VulkanRendererGpu.hpp"
#include "spark/render/VulkanSceneVertexLayout.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Spark {

namespace {

void DestroyStagingBuffer(
        VkDevice device,
        VkBuffer& buffer,
        VkDeviceMemory& memory,
        void*& mapped) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (mapped != nullptr) {
        vkUnmapMemory(device, memory);
        mapped = nullptr;
    }
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}

}  // namespace

void VulkanCustomMeshPool::DestroyBufferSet(BufferSet& set, VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (set.vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, set.vertexBuffer, nullptr);
        set.vertexBuffer = VK_NULL_HANDLE;
    }
    if (set.vertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, set.vertexMemory, nullptr);
        set.vertexMemory = VK_NULL_HANDLE;
    }
    if (set.indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, set.indexBuffer, nullptr);
        set.indexBuffer = VK_NULL_HANDLE;
    }
    if (set.indexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, set.indexMemory, nullptr);
        set.indexMemory = VK_NULL_HANDLE;
    }
    set.vertexCapacityBytes = 0;
    set.indexCapacityBytes = 0;
}

void VulkanCustomMeshPool::QueueRetire(BufferSet&& set, const std::uint64_t safeAfterFrame) {
    if (set.vertexBuffer == VK_NULL_HANDLE && set.indexBuffer == VK_NULL_HANDLE) {
        return;
    }
    RetiredSet entry{};
    entry.buffers = set;
    entry.safeAfterFrame = safeAfterFrame;
    retired_.PushBack(entry);
    set = {};
}

void VulkanCustomMeshPool::CreateResources(const VkPhysicalDevice physicalDevice, const VkDevice device) {
    DestroyResources(device);
    physicalDevice_ = physicalDevice;
    device_ = device;
}

void VulkanCustomMeshPool::DestroyResources(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    for (std::size_t i = 0; i < retired_.GetSize(); ++i) {
        DestroyBufferSet(retired_[i].buffers, device);
    }
    retired_.Clear();
    DestroyBufferSet(active_, device);
    DestroyStagingBuffer(device, stagingVertex_, stagingVertexMemory_, stagingVertexMapped_);
    DestroyStagingBuffer(device, stagingIndex_, stagingIndexMemory_, stagingIndexMapped_);
    stagingVertexCapacity_ = 0;
    stagingIndexCapacity_ = 0;
    pendingVertexBytes_ = 0;
    pendingIndexBytes_ = 0;
    uploadPending_ = false;
    lastFingerprint_ = 0;
    rigidSlices_.Clear();
    skinnedSlices_.Clear();
    physicalDevice_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
}

void VulkanCustomMeshPool::ReleaseRetiredBuffers(const std::uint64_t frameCounter) {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }
    std::size_t write = 0;
    for (std::size_t i = 0; i < retired_.GetSize(); ++i) {
        if (retired_[i].safeAfterFrame <= frameCounter) {
            DestroyBufferSet(retired_[i].buffers, device_);
        } else {
            if (write != i) {
                retired_[write] = retired_[i];
            }
            ++write;
        }
    }
    retired_.Resize(write);
}

void VulkanCustomMeshPool::EnsureStagingCapacity(const VkDeviceSize vertexBytes, const VkDeviceSize indexBytes) {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }
    if (vertexBytes > stagingVertexCapacity_) {
        DestroyStagingBuffer(device_, stagingVertex_, stagingVertexMemory_, stagingVertexMapped_);
        stagingVertexCapacity_ = vertexBytes;
        VulkanRendererGpu::CreateBuffer(
                physicalDevice_,
                device_,
                stagingVertexCapacity_,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingVertex_,
                stagingVertexMemory_);
        if (vkMapMemory(device_, stagingVertexMemory_, 0, stagingVertexCapacity_, 0, &stagingVertexMapped_) !=
                VK_SUCCESS) {
            throw std::runtime_error("VulkanCustomMeshPool: map staging vertex failed");
        }
    }
    if (indexBytes > stagingIndexCapacity_) {
        DestroyStagingBuffer(device_, stagingIndex_, stagingIndexMemory_, stagingIndexMapped_);
        stagingIndexCapacity_ = indexBytes;
        VulkanRendererGpu::CreateBuffer(
                physicalDevice_,
                device_,
                stagingIndexCapacity_,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingIndex_,
                stagingIndexMemory_);
        if (vkMapMemory(device_, stagingIndexMemory_, 0, stagingIndexCapacity_, 0, &stagingIndexMapped_) !=
                VK_SUCCESS) {
            throw std::runtime_error("VulkanCustomMeshPool: map staging index failed");
        }
    }
}

bool VulkanCustomMeshPool::EnsureDeviceCapacity(
        const VkDeviceSize vertexBytes,
        const VkDeviceSize indexBytes,
        const std::uint64_t frameCounter,
        const std::uint32_t maxFramesInFlight) {
    if (active_.vertexBuffer != VK_NULL_HANDLE || active_.indexBuffer != VK_NULL_HANDLE) {
        QueueRetire(MoveTemp(active_), frameCounter + static_cast<std::uint64_t>(maxFramesInFlight));
    }

    const VkDeviceSize vbAlloc = std::max(vertexBytes, active_.vertexCapacityBytes);
    const VkDeviceSize ibAlloc = std::max(indexBytes, active_.indexCapacityBytes);
    VulkanRendererGpu::CreateBuffer(
            physicalDevice_,
            device_,
            vbAlloc,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            active_.vertexBuffer,
            active_.vertexMemory);
    VulkanRendererGpu::CreateBuffer(
            physicalDevice_,
            device_,
            ibAlloc,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            active_.indexBuffer,
            active_.indexMemory);
    active_.vertexCapacityBytes = vbAlloc;
    active_.indexCapacityBytes = ibAlloc;
    return true;
}

std::uint64_t VulkanCustomMeshPool::ComputeFingerprint(const SceneRenderParams& scene) const {
    std::uint64_t h = Fnv64Begin();
    auto mixDraw = [&](const SceneDrawItem& d) {
        if (d.mesh != SceneMeshSlot::Custom) {
            return;
        }
        if (d.skinnedMesh) {
            const SkinnedMesh* p = d.skinnedMesh.Get();
            Fnv64Mix(h, reinterpret_cast<std::uintptr_t>(p));
            if (p != nullptr) {
                Fnv64Mix(h, static_cast<std::uint64_t>(p->GetVertices().GetSize()));
                Fnv64Mix(h, static_cast<std::uint64_t>(p->GetIndices().GetSize()));
            }
            return;
        }
        if (!d.customMesh) {
            return;
        }
        const Mesh* p = d.customMesh.Get();
        Fnv64Mix(h, reinterpret_cast<std::uintptr_t>(p));
        if (p != nullptr) {
            Fnv64Mix(h, static_cast<std::uint64_t>(p->GetVertices().GetSize()));
            Fnv64Mix(h, static_cast<std::uint64_t>(p->GetIndices().GetSize()));
        }
    };
    for (std::size_t i = 0; i < scene.draws.GetSize(); ++i) {
        mixDraw(scene.draws[i]);
    }
    for (std::size_t i = 0; i < scene.transparentDraws.GetSize(); ++i) {
        mixDraw(scene.transparentDraws[i]);
    }
    return h;
}

void VulkanCustomMeshPool::PackSceneGeometry(
        const SceneRenderParams& scene,
        Array<float>& interleaved,
        Array<std::uint32_t>& meshIndices) {
    Array<const Mesh*> uniqueRigid;
    auto collectRigid = [&](const SceneDrawItem& d) {
        if (d.mesh != SceneMeshSlot::Custom || d.skinnedMesh || !d.customMesh) {
            return;
        }
        const Mesh* mp = d.customMesh.Get();
        bool seen = false;
        for (std::size_t u = 0; u < uniqueRigid.GetSize(); ++u) {
            if (uniqueRigid[u] == mp) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            uniqueRigid.PushBack(mp);
        }
    };
    for (std::size_t i = 0; i < scene.draws.GetSize(); ++i) {
        collectRigid(scene.draws[i]);
    }
    for (std::size_t i = 0; i < scene.transparentDraws.GetSize(); ++i) {
        collectRigid(scene.transparentDraws[i]);
    }

    Array<const SkinnedMesh*> uniqueSkinned;
    auto collectSkinned = [&](const SceneDrawItem& d) {
        if (d.mesh != SceneMeshSlot::Custom || !d.skinnedMesh) {
            return;
        }
        const SkinnedMesh* sp = d.skinnedMesh.Get();
        bool seen = false;
        for (std::size_t u = 0; u < uniqueSkinned.GetSize(); ++u) {
            if (uniqueSkinned[u] == sp) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            uniqueSkinned.PushBack(sp);
        }
    };
    for (std::size_t i = 0; i < scene.draws.GetSize(); ++i) {
        collectSkinned(scene.draws[i]);
    }
    for (std::size_t i = 0; i < scene.transparentDraws.GetSize(); ++i) {
        collectSkinned(scene.transparentDraws[i]);
    }

    interleaved.Clear();
    meshIndices.Clear();
    rigidSlices_.Clear();
    skinnedSlices_.Clear();

    for (std::size_t ui = 0; ui < uniqueRigid.GetSize(); ++ui) {
        const Mesh* mp = uniqueRigid[ui];
        if (mp == nullptr || mp->GetVertices().IsEmpty() || mp->GetIndices().IsEmpty()) {
            continue;
        }
        const std::uint32_t vBase =
                static_cast<std::uint32_t>(interleaved.GetSize() / VulkanSceneVertexLayout::kFloatsPerVertex);
        const std::uint32_t firstIdx = static_cast<std::uint32_t>(meshIndices.GetSize());
        const auto& verts = mp->GetVertices();
        const auto& inds = mp->GetIndices();
        for (std::size_t vi = 0; vi < verts.GetSize(); ++vi) {
            VulkanRendererGpu::AppendRigidMeshVertexToInterleaved(verts[vi], interleaved);
        }
        for (std::size_t ii = 0; ii < inds.GetSize(); ++ii) {
            meshIndices.PushBack(vBase + inds[ii]);
        }
        CustomMeshGpuSlice slice{};
        slice.firstIndex = firstIdx;
        slice.indexCount = static_cast<std::uint32_t>(inds.GetSize());
        slice.vertexOffset = 0;
        rigidSlices_.Add(mp, slice);
    }

    for (std::size_t ui = 0; ui < uniqueSkinned.GetSize(); ++ui) {
        const SkinnedMesh* sp = uniqueSkinned[ui];
        if (sp == nullptr || sp->GetVertices().IsEmpty() || sp->GetIndices().IsEmpty()) {
            continue;
        }
        const std::uint32_t vBase =
                static_cast<std::uint32_t>(interleaved.GetSize() / VulkanSceneVertexLayout::kFloatsPerVertex);
        const std::uint32_t firstIdx = static_cast<std::uint32_t>(meshIndices.GetSize());
        const auto& verts = sp->GetVertices();
        const auto& inds = sp->GetIndices();
        for (std::size_t vi = 0; vi < verts.GetSize(); ++vi) {
            VulkanRendererGpu::AppendSkinnedVertexToInterleaved(verts[vi], interleaved);
        }
        for (std::size_t ii = 0; ii < inds.GetSize(); ++ii) {
            meshIndices.PushBack(vBase + inds[ii]);
        }
        CustomMeshGpuSlice slice{};
        slice.firstIndex = firstIdx;
        slice.indexCount = static_cast<std::uint32_t>(inds.GetSize());
        slice.vertexOffset = 0;
        skinnedSlices_.Add(sp, slice);
    }
}

void VulkanCustomMeshPool::UpdateFromScene(
        const SceneRenderParams& scene,
        const std::uint64_t frameCounter,
        const std::uint32_t maxFramesInFlight) {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }

    bool anyCustom = false;
    auto inspectCustom = [&](const SceneDrawItem& d) {
        if (d.mesh != SceneMeshSlot::Custom) {
            return;
        }
        if (d.skinnedMesh || d.customMesh) {
            anyCustom = true;
        }
    };
    for (std::size_t i = 0; i < scene.draws.GetSize() && !anyCustom; ++i) {
        inspectCustom(scene.draws[i]);
    }
    for (std::size_t i = 0; i < scene.transparentDraws.GetSize() && !anyCustom; ++i) {
        inspectCustom(scene.transparentDraws[i]);
    }

    if (!anyCustom) {
        if (active_.vertexBuffer != VK_NULL_HANDLE || active_.indexBuffer != VK_NULL_HANDLE) {
            QueueRetire(MoveTemp(active_), frameCounter + static_cast<std::uint64_t>(maxFramesInFlight));
        }
        rigidSlices_.Clear();
        skinnedSlices_.Clear();
        lastFingerprint_ = 0;
        uploadPending_ = false;
        pendingVertexBytes_ = 0;
        pendingIndexBytes_ = 0;
        return;
    }

    const std::uint64_t fp = ComputeFingerprint(scene);
    if (fp == lastFingerprint_ && active_.vertexBuffer != VK_NULL_HANDLE && !uploadPending_) {
        return;
    }
    lastFingerprint_ = fp;

    Array<float> interleaved;
    Array<std::uint32_t> meshIndices;
    PackSceneGeometry(scene, interleaved, meshIndices);

    const VkDeviceSize vbSize = sizeof(float) * static_cast<std::size_t>(interleaved.GetSize());
    const VkDeviceSize ibSize = sizeof(std::uint32_t) * static_cast<std::size_t>(meshIndices.GetSize());
    if (vbSize == 0 || ibSize == 0) {
        if (active_.vertexBuffer != VK_NULL_HANDLE || active_.indexBuffer != VK_NULL_HANDLE) {
            QueueRetire(MoveTemp(active_), frameCounter + static_cast<std::uint64_t>(maxFramesInFlight));
        }
        rigidSlices_.Clear();
        skinnedSlices_.Clear();
        lastFingerprint_ = 0;
        uploadPending_ = false;
        pendingVertexBytes_ = 0;
        pendingIndexBytes_ = 0;
        return;
    }

    EnsureStagingCapacity(vbSize, ibSize);
    std::memcpy(stagingVertexMapped_, interleaved.GetData(), static_cast<std::size_t>(vbSize));
    std::memcpy(stagingIndexMapped_, meshIndices.GetData(), static_cast<std::size_t>(ibSize));
    pendingVertexBytes_ = vbSize;
    pendingIndexBytes_ = ibSize;

    EnsureDeviceCapacity(vbSize, ibSize, frameCounter, maxFramesInFlight);
    uploadPending_ = true;
}

void VulkanCustomMeshPool::RecordUploads(const VkCommandBuffer commandBuffer) {
    if (!uploadPending_ || device_ == VK_NULL_HANDLE || commandBuffer == VK_NULL_HANDLE) {
        return;
    }
    if (active_.vertexBuffer == VK_NULL_HANDLE || active_.indexBuffer == VK_NULL_HANDLE ||
        stagingVertex_ == VK_NULL_HANDLE || stagingIndex_ == VK_NULL_HANDLE) {
        uploadPending_ = false;
        return;
    }

    VkBufferCopy vtxCopy{};
    vtxCopy.size = pendingVertexBytes_;
    vkCmdCopyBuffer(commandBuffer, stagingVertex_, active_.vertexBuffer, 1, &vtxCopy);

    VkBufferCopy idxCopy{};
    idxCopy.size = pendingIndexBytes_;
    vkCmdCopyBuffer(commandBuffer, stagingIndex_, active_.indexBuffer, 1, &idxCopy);

    VkBufferMemoryBarrier barriers[2]{};
    barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].buffer = active_.vertexBuffer;
    barriers[0].offset = 0;
    barriers[0].size = pendingVertexBytes_;

    barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].buffer = active_.indexBuffer;
    barriers[1].offset = 0;
    barriers[1].size = pendingIndexBytes_;

    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            0,
            0,
            nullptr,
            2,
            barriers,
            0,
            nullptr);

    uploadPending_ = false;
}

VulkanCustomMeshPool::Bindings VulkanCustomMeshPool::GetBindings() const noexcept {
    return Bindings{
            .vertexBuffer = active_.vertexBuffer,
            .indexBuffer = active_.indexBuffer,
    };
}

void VulkanCustomMeshPool::FillCustomDrawPacked(
        const SceneRenderParams& scene,
        Array<CustomMeshGpuSlice>& outOpaquePacked,
        Array<CustomMeshGpuSlice>& outTransparentPacked) const {
    outOpaquePacked.Resize(scene.draws.GetSize());
    outTransparentPacked.Resize(scene.transparentDraws.GetSize());
    for (std::size_t i = 0; i < scene.draws.GetSize(); ++i) {
        outOpaquePacked[i] = CustomMeshGpuSlice{};
        const SceneDrawItem& d = scene.draws[i];
        if (d.mesh != SceneMeshSlot::Custom) {
            continue;
        }
        if (d.skinnedMesh) {
            if (const CustomMeshGpuSlice* s = skinnedSlices_.Find(d.skinnedMesh.Get())) {
                outOpaquePacked[i] = *s;
            }
            continue;
        }
        if (d.customMesh) {
            if (const CustomMeshGpuSlice* s = rigidSlices_.Find(d.customMesh.Get())) {
                outOpaquePacked[i] = *s;
            }
        }
    }
    for (std::size_t i = 0; i < scene.transparentDraws.GetSize(); ++i) {
        outTransparentPacked[i] = CustomMeshGpuSlice{};
        const SceneDrawItem& d = scene.transparentDraws[i];
        if (d.mesh != SceneMeshSlot::Custom) {
            continue;
        }
        if (d.skinnedMesh) {
            if (const CustomMeshGpuSlice* s = skinnedSlices_.Find(d.skinnedMesh.Get())) {
                outTransparentPacked[i] = *s;
            }
            continue;
        }
        if (d.customMesh) {
            if (const CustomMeshGpuSlice* s = rigidSlices_.Find(d.customMesh.Get())) {
                outTransparentPacked[i] = *s;
            }
        }
    }
}

}  // namespace Spark
