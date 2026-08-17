#include "spark/render/scene/VulkanSceneDescriptors.hpp"

#include "spark/render/scene/VulkanClusteredForwardLights.hpp"
#include "spark/render/scene/VulkanClusteredLightGpu.hpp"
#include "spark/render/shadow/VulkanDirectionalShadowPass.hpp"
#include "spark/render/shadow/VulkanPunctualShadowGpu.hpp"
#include "spark/render/shadow/VulkanPunctualShadowPass.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/scene/VulkanSceneTextureUploader.hpp"
#include "spark/render/scene/VulkanSceneUniformGpu.hpp"
#include "spark/render/sprites2d/VulkanSpriteInstanceGpu.hpp"
#include "spark/render/sprites2d/VulkanSpritePass.hpp"

#include <stdexcept>

namespace Spark {

void VulkanSceneDescriptors::CreateSetLayout(VkDevice device) {
    VkDescriptorSetLayoutBinding bindings[11]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[6].binding = 6;
    bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[6].descriptorCount = 1;
    bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[7].binding = 7;
    bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[7].descriptorCount = 1;
    bindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[8].binding = 8;
    bindings[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[8].descriptorCount = 1;
    bindings[8].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[9].binding = 9;
    bindings[9].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[9].descriptorCount = 1;
    bindings[9].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[10].binding = 10;
    bindings[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[10].descriptorCount = 1;
    bindings[10].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 11;
    layoutInfo.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorSetLayout failed");
    }
}

void VulkanSceneDescriptors::CreateUniformBuffers(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        std::uint32_t framesInFlight) {
    constexpr VkDeviceSize bufferSize = kSceneUniformGpuBytes;
    uniformBuffers.Resize(framesInFlight);
    uniformBuffersMemory.Resize(framesInFlight);
    uniformBuffersMapped.Resize(framesInFlight);

    for (std::size_t i = 0; i < framesInFlight; ++i) {
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[i],
                uniformBuffersMemory[i]);
        if (vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("vkMapMemory UBO failed");
        }
    }
}

void VulkanSceneDescriptors::CreateSkinSsboBuffers(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        std::uint32_t framesInFlight) {
    skinSsboBuffers.Resize(framesInFlight);
    skinSsboMemory.Resize(framesInFlight);
    skinSsboMapped.Resize(framesInFlight);
    for (std::size_t i = 0; i < framesInFlight; ++i) {
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kSkinSsboBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                skinSsboBuffers[i],
                skinSsboMemory[i]);
        if (vkMapMemory(device, skinSsboMemory[i], 0, kSkinSsboBytes, 0, &skinSsboMapped[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkMapMemory skin SSBO failed");
        }
    }
}

void VulkanSceneDescriptors::CreatePoolAndSets(
        VkDevice device,
        std::uint32_t framesInFlight,
        const BindingSources& sources) {
    VkDescriptorPoolSize poolSizes[3]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = framesInFlight;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = framesInFlight * 5;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = framesInFlight * 5;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 3;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = framesInFlight;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorPool failed");
    }

    Array<VkDescriptorSetLayout> layouts;
    layouts.Resize(framesInFlight);
    for (std::size_t li = 0; li < framesInFlight; ++li) {
        layouts[li] = descriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = framesInFlight;
    allocInfo.pSetLayouts = layouts.GetData();

    descriptorSets.Resize(framesInFlight);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.GetData()) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateDescriptorSets failed");
    }

    constexpr VkDeviceSize bufferSize = kSceneUniformGpuBytes;
    for (std::size_t i = 0; i < framesInFlight; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = sources.sceneTextureUploader.ArrayView();
        imageInfo.sampler = sources.sceneTextureUploader.Sampler();

        VkWriteDescriptorSet texWrite{};
        texWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        texWrite.dstSet = descriptorSets[i];
        texWrite.dstBinding = 1;
        texWrite.dstArrayElement = 0;
        texWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texWrite.descriptorCount = 1;
        texWrite.pImageInfo = &imageInfo;

        VkDescriptorImageInfo spriteImageInfo{};
        spriteImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        spriteImageInfo.imageView = sources.sceneTextureUploader.ArrayView();
        spriteImageInfo.sampler = sources.sceneTextureUploader.SpriteSampler();

        VkWriteDescriptorSet spriteTexWrite{};
        spriteTexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        spriteTexWrite.dstSet = descriptorSets[i];
        spriteTexWrite.dstBinding = 10;
        spriteTexWrite.dstArrayElement = 0;
        spriteTexWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        spriteTexWrite.descriptorCount = 1;
        spriteTexWrite.pImageInfo = &spriteImageInfo;

        VkDescriptorBufferInfo skinInfo{};
        skinInfo.buffer = skinSsboBuffers[i];
        skinInfo.offset = 0;
        skinInfo.range = kSkinSsboBytes;

        VkWriteDescriptorSet skinWrite{};
        skinWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skinWrite.dstSet = descriptorSets[i];
        skinWrite.dstBinding = 2;
        skinWrite.dstArrayElement = 0;
        skinWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        skinWrite.descriptorCount = 1;
        skinWrite.pBufferInfo = &skinInfo;

        VkDescriptorBufferInfo lightsInfo{};
        lightsInfo.buffer = sources.clusteredForwardLights.LightsBuffer(static_cast<std::uint32_t>(i));
        lightsInfo.offset = 0;
        lightsInfo.range = kClusterLightsGpuBytes;

        VkWriteDescriptorSet lightsWrite{};
        lightsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightsWrite.dstSet = descriptorSets[i];
        lightsWrite.dstBinding = 4;
        lightsWrite.dstArrayElement = 0;
        lightsWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        lightsWrite.descriptorCount = 1;
        lightsWrite.pBufferInfo = &lightsInfo;

        VkDescriptorBufferInfo clusterInfo{};
        clusterInfo.buffer = sources.clusteredForwardLights.ClusterBuffer(static_cast<std::uint32_t>(i));
        clusterInfo.offset = 0;
        clusterInfo.range = kClusterGridGpuBytes;

        VkWriteDescriptorSet clusterWrite{};
        clusterWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        clusterWrite.dstSet = descriptorSets[i];
        clusterWrite.dstBinding = 5;
        clusterWrite.dstArrayElement = 0;
        clusterWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        clusterWrite.descriptorCount = 1;
        clusterWrite.pBufferInfo = &clusterInfo;

        VkDescriptorBufferInfo punctualSsboInfo{};
        punctualSsboInfo.buffer = sources.punctualShadow.SsboBuffer(static_cast<std::uint32_t>(i));
        punctualSsboInfo.offset = 0;
        punctualSsboInfo.range = kPunctualShadowGpuBytes;

        VkWriteDescriptorSet punctualSsboWrite{};
        punctualSsboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        punctualSsboWrite.dstSet = descriptorSets[i];
        punctualSsboWrite.dstBinding = 6;
        punctualSsboWrite.dstArrayElement = 0;
        punctualSsboWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        punctualSsboWrite.descriptorCount = 1;
        punctualSsboWrite.pBufferInfo = &punctualSsboInfo;

        VkDescriptorBufferInfo spriteInstanceInfo{};
        spriteInstanceInfo.buffer = sources.spritePass.InstanceBuffer(static_cast<std::uint32_t>(i));
        spriteInstanceInfo.offset = 0;
        spriteInstanceInfo.range = static_cast<VkDeviceSize>(kQuadInstanceSsboBytes);

        VkWriteDescriptorSet spriteInstanceWrite{};
        spriteInstanceWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        spriteInstanceWrite.dstSet = descriptorSets[i];
        spriteInstanceWrite.dstBinding = 9;
        spriteInstanceWrite.dstArrayElement = 0;
        spriteInstanceWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        spriteInstanceWrite.descriptorCount = 1;
        spriteInstanceWrite.pBufferInfo = &spriteInstanceInfo;

        const std::size_t shadowFlight = i;
        const bool hasSunShadow =
                sources.directionalShadow.HasFlightDepthView(static_cast<std::uint32_t>(shadowFlight));
        const bool hasPunctualShadow =
                sources.punctualShadow.HasFlightResources(static_cast<std::uint32_t>(shadowFlight));

        VkDescriptorImageInfo sunShadowInfo{};
        VkWriteDescriptorSet sunShadowWrite{};
        if (hasSunShadow) {
            sunShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            sunShadowInfo.imageView =
                    sources.directionalShadow.Flight(static_cast<std::uint32_t>(shadowFlight)).depthView;
            sunShadowInfo.sampler = sources.directionalShadow.CompareSampler();
            sunShadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            sunShadowWrite.dstSet = descriptorSets[i];
            sunShadowWrite.dstBinding = 3;
            sunShadowWrite.dstArrayElement = 0;
            sunShadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            sunShadowWrite.descriptorCount = 1;
            sunShadowWrite.pImageInfo = &sunShadowInfo;
        }

        VkDescriptorImageInfo spotShadowInfo{};
        VkWriteDescriptorSet spotShadowWrite{};
        VkDescriptorImageInfo pointShadowInfo{};
        VkWriteDescriptorSet pointShadowWrite{};
        if (hasPunctualShadow) {
            const VulkanPunctualShadowPass::FlightTarget& pf =
                    sources.punctualShadow.Flight(static_cast<std::uint32_t>(shadowFlight));
            spotShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            spotShadowInfo.imageView = pf.spot.depthView;
            spotShadowInfo.sampler = sources.punctualShadow.CompareSampler();
            spotShadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            spotShadowWrite.dstSet = descriptorSets[i];
            spotShadowWrite.dstBinding = 7;
            spotShadowWrite.dstArrayElement = 0;
            spotShadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            spotShadowWrite.descriptorCount = 1;
            spotShadowWrite.pImageInfo = &spotShadowInfo;

            pointShadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            pointShadowInfo.imageView = pf.point.depthArrayView;
            pointShadowInfo.sampler = sources.punctualShadow.CompareSampler();
            pointShadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            pointShadowWrite.dstSet = descriptorSets[i];
            pointShadowWrite.dstBinding = 8;
            pointShadowWrite.dstArrayElement = 0;
            pointShadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            pointShadowWrite.descriptorCount = 1;
            pointShadowWrite.pImageInfo = &pointShadowInfo;
        }

        if (hasSunShadow && hasPunctualShadow) {
            const VkWriteDescriptorSet writes[] = {descriptorWrite,
                    texWrite,
                    skinWrite,
                    sunShadowWrite,
                    lightsWrite,
                    clusterWrite,
                    punctualSsboWrite,
                    spotShadowWrite,
                    pointShadowWrite,
                    spriteInstanceWrite,
                    spriteTexWrite};
            vkUpdateDescriptorSets(device, 11, writes, 0, nullptr);
        } else if (hasSunShadow) {
            const VkWriteDescriptorSet writes[] = {descriptorWrite,
                    texWrite,
                    skinWrite,
                    sunShadowWrite,
                    lightsWrite,
                    clusterWrite,
                    punctualSsboWrite,
                    spriteInstanceWrite,
                    spriteTexWrite};
            vkUpdateDescriptorSets(device, 9, writes, 0, nullptr);
        } else if (hasPunctualShadow) {
            const VkWriteDescriptorSet writes[] = {descriptorWrite,
                    texWrite,
                    skinWrite,
                    lightsWrite,
                    clusterWrite,
                    punctualSsboWrite,
                    spotShadowWrite,
                    pointShadowWrite,
                    spriteInstanceWrite,
                    spriteTexWrite};
            vkUpdateDescriptorSets(device, 10, writes, 0, nullptr);
        } else {
            const VkWriteDescriptorSet writes[] = {descriptorWrite,
                    texWrite,
                    skinWrite,
                    lightsWrite,
                    clusterWrite,
                    punctualSsboWrite,
                    spriteInstanceWrite,
                    spriteTexWrite};
            vkUpdateDescriptorSets(device, 8, writes, 0, nullptr);
        }
    }
}

void VulkanSceneDescriptors::Destroy(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (std::size_t i = 0; i < uniformBuffers.GetSize(); ++i) {
        if (uniformBuffersMapped[i] != nullptr) {
            vkUnmapMemory(device, uniformBuffersMemory[i]);
            uniformBuffersMapped[i] = nullptr;
        }
        if (uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            uniformBuffers[i] = VK_NULL_HANDLE;
        }
        if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
            uniformBuffersMemory[i] = VK_NULL_HANDLE;
        }
    }
    uniformBuffers.Clear();
    uniformBuffersMemory.Clear();
    uniformBuffersMapped.Clear();

    for (std::size_t i = 0; i < skinSsboBuffers.GetSize(); ++i) {
        if (skinSsboMapped[i] != nullptr) {
            vkUnmapMemory(device, skinSsboMemory[i]);
            skinSsboMapped[i] = nullptr;
        }
        if (skinSsboBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, skinSsboBuffers[i], nullptr);
            skinSsboBuffers[i] = VK_NULL_HANDLE;
        }
        if (skinSsboMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, skinSsboMemory[i], nullptr);
            skinSsboMemory[i] = VK_NULL_HANDLE;
        }
    }
    skinSsboBuffers.Clear();
    skinSsboMemory.Clear();
    skinSsboMapped.Clear();

    descriptorSets.Clear();
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
}

}  // namespace Spark
