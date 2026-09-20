#include "spark/render/scene/VulkanRenderTargetRegistry.hpp"

#include "spark/scene/render/RenderTexture.hpp"

namespace Spark {

VulkanRenderTargetRegistry::VulkanRenderTargetView::VulkanRenderTargetView(
        SharedPtr<RenderTexture> textureIn,
        VulkanRenderTargetRegistry& ownerIn) noexcept
        : texture(textureIn), owner(ownerIn) {
    if (Entry* entry = owner.FindEntry(*texture)) {
        ++entry->viewRefCount;
    }
}

VulkanRenderTargetRegistry::VulkanRenderTargetView::~VulkanRenderTargetView() {
    if (texture) {
        owner.DropView(*texture);
    }
}

void VulkanRenderTargetRegistry::BindDevice(
        const VkPhysicalDevice physicalDeviceIn,
        const VkDevice deviceIn,
        const VkRenderPass hdrRenderPassIn,
        const VkFormat depthFormatIn) noexcept {
    physicalDevice = physicalDeviceIn;
    device = deviceIn;
    hdrRenderPass = hdrRenderPassIn;
    depthFormat = depthFormatIn;
}

void VulkanRenderTargetRegistry::DestroyAll(const VkDevice deviceIn) noexcept {
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].gpu != nullptr) {
            entries[i].gpu->Destroy(deviceIn);
            entries[i].gpu.Reset();
        }
        entries[i].texture.Reset();
        entries[i].viewRefCount = 0;
    }
    entries.Clear();
    indexByTexture.Clear();
}

VulkanRenderTargetRegistry::Entry* VulkanRenderTargetRegistry::FindEntry(const RenderTexture& texture) noexcept {
    const auto* index = indexByTexture.Find(&texture);
    if (index == nullptr) {
        return nullptr;
    }
    return &entries[*index];
}

const VulkanRenderTargetRegistry::Entry* VulkanRenderTargetRegistry::FindEntry(
        const RenderTexture& texture) const noexcept {
    const auto* index = indexByTexture.Find(&texture);
    if (index == nullptr) {
        return nullptr;
    }
    return &entries[*index];
}

void VulkanRenderTargetRegistry::AllocateGpuForEntry(Entry& entry) {
    if (!entry.texture || device == VK_NULL_HANDLE || hdrRenderPass == VK_NULL_HANDLE) {
        return;
    }
    if (entry.gpu == nullptr) {
        entry.gpu = MakeUnique<VulkanOffscreenRenderTarget>();
    }
    VulkanOffscreenRenderTarget& gpu = *entry.gpu;
    const RenderTexture& texture = *entry.texture;
    if (!gpu.IsAllocated() || gpu.AllocatedToken() != texture.GetGpuAllocationToken() ||
        gpu.Extent().width != texture.GetWidth() || gpu.Extent().height != texture.GetHeight()) {
        gpu.Create(physicalDevice, device, hdrRenderPass, depthFormat, texture.GetDesc());
        gpu.SetAllocatedToken(texture.GetGpuAllocationToken());
    }
}

void VulkanRenderTargetRegistry::EnsureGpuResources(RenderTexture& texture) {
    if (device == VK_NULL_HANDLE || hdrRenderPass == VK_NULL_HANDLE ||
        !RenderTexture::IsValidExtent(texture.GetWidth(), texture.GetHeight())) {
        return;
    }
    Entry* entry = FindEntry(texture);
    if (entry == nullptr) {
        return;
    }
    AllocateGpuForEntry(*entry);
}

SharedPtr<IRenderTarget> VulkanRenderTargetRegistry::CreateRenderTarget(SharedPtr<RenderTexture> texture) {
    if (!texture || device == VK_NULL_HANDLE || hdrRenderPass == VK_NULL_HANDLE) {
        return {};
    }

    Entry* entry = FindEntry(*texture);
    if (entry == nullptr) {
        Entry created{};
        created.texture = texture;
        const std::size_t slot = entries.GetSize();
        entries.PushBack(MoveTemp(created));
        indexByTexture.Add(texture.Get(), slot);
        entry = &entries[slot];
    }

    AllocateGpuForEntry(*entry);
    return SharedPtr<IRenderTarget>(new VulkanRenderTargetView(texture, *this));
}

void VulkanRenderTargetRegistry::ReleaseRenderTarget(const RenderTexture& texture) noexcept {
    const auto* index = indexByTexture.Find(&texture);
    if (index == nullptr) {
        return;
    }
    Entry& entry = entries[*index];
    if (entry.viewRefCount > 0) {
        return;
    }
    if (entry.gpu != nullptr) {
        entry.gpu->Destroy(device);
        entry.gpu.Reset();
    }
    const std::size_t slot = *index;
    entries.RemoveAt(slot);
    indexByTexture.Clear();
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].texture) {
            indexByTexture.Add(entries[i].texture.Get(), i);
        }
    }
}

void VulkanRenderTargetRegistry::DropView(const RenderTexture& texture) noexcept {
    Entry* entry = FindEntry(texture);
    if (entry == nullptr || entry->viewRefCount == 0) {
        return;
    }
    --entry->viewRefCount;
    if (entry->viewRefCount == 0) {
        ReleaseRenderTarget(texture);
    }
}

VulkanOffscreenRenderTarget* VulkanRenderTargetRegistry::TryGetGpu(const RenderTexture& texture) noexcept {
    const Entry* entry = FindEntry(texture);
    if (entry == nullptr || !entry->gpu) {
        return nullptr;
    }
    return entry->gpu.Get();
}

const VulkanOffscreenRenderTarget* VulkanRenderTargetRegistry::TryGetGpu(const RenderTexture& texture) const noexcept {
    const Entry* entry = FindEntry(texture);
    if (entry == nullptr || !entry->gpu) {
        return nullptr;
    }
    return entry->gpu.Get();
}

}  // namespace Spark
