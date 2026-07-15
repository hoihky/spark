#include "spark/render/VulkanHdrTonemapPass.hpp"

#include "spark/render/VulkanRendererGpu.hpp"

#include <cstring>
#include <stdexcept>

namespace Spark {

void VulkanHdrTonemapPass::CreateRenderPass(VkDevice device, VkFormat depthFormat) {
    DestroyRenderPass(device);

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = kColorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    const VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 2;
    rpInfo.pAttachments = attachments;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    rpInfo.dependencyCount = 1;
    rpInfo.pDependencies = &dep;
    if (vkCreateRenderPass(device, &rpInfo, nullptr, &hdrRenderPass_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateRenderPass (HDR) failed");
    }
}

void VulkanHdrTonemapPass::DestroyRenderPass(VkDevice device) {
    if (hdrRenderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, hdrRenderPass_, nullptr);
        hdrRenderPass_ = VK_NULL_HANDLE;
    }
}

void VulkanHdrTonemapPass::RecreateFlightTargets(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D extent,
        std::uint32_t framesInFlight,
        const VkImageView* depthViews,
        std::size_t depthViewCount) {
    DestroyFlightTargets(device);
    if (extent.width == 0 || extent.height == 0 || hdrRenderPass_ == VK_NULL_HANDLE ||
        depthViewCount != static_cast<std::size_t>(framesInFlight)) {
        return;
    }

    flights_.Resize(framesInFlight);
    for (std::size_t fi = 0; fi < framesInFlight; ++fi) {
        FlightTarget& hdr = flights_[fi];
        VulkanRendererGpu::CreateImage(
                physicalDevice,
                device,
                extent.width,
                extent.height,
                kColorFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                hdr.colorImage,
                hdr.colorMemory);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = hdr.colorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = kColorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &viewInfo, nullptr, &hdr.colorView) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateImageView (HDR) failed");
        }
        hdr.colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        const VkImageView fbAttachments[] = {hdr.colorView, depthViews[fi]};
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = hdrRenderPass_;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = fbAttachments;
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;
        if (vkCreateFramebuffer(device, &fbInfo, nullptr, &hdr.framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateFramebuffer (HDR) failed");
        }
    }

    if (colorSampler_ == VK_NULL_HANDLE) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0F;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        if (vkCreateSampler(device, &samplerInfo, nullptr, &colorSampler_) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateSampler (HDR) failed");
        }
    }

    if (!tonemapDescriptorSets_.IsEmpty()) {
        for (std::uint32_t fi = 0; fi < framesInFlight; ++fi) {
            UpdateTonemapDescriptor(device, fi);
        }
    }
}

void VulkanHdrTonemapPass::DestroyFlightTargets(VkDevice device) {
    for (std::size_t fi = 0; fi < flights_.GetSize(); ++fi) {
        FlightTarget& hdr = flights_[fi];
        if (hdr.framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, hdr.framebuffer, nullptr);
            hdr.framebuffer = VK_NULL_HANDLE;
        }
        if (hdr.colorView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, hdr.colorView, nullptr);
            hdr.colorView = VK_NULL_HANDLE;
        }
        if (hdr.colorImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, hdr.colorImage, nullptr);
            hdr.colorImage = VK_NULL_HANDLE;
        }
        if (hdr.colorMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, hdr.colorMemory, nullptr);
            hdr.colorMemory = VK_NULL_HANDLE;
        }
        hdr.colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    flights_.Clear();
    if (colorSampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device, colorSampler_, nullptr);
        colorSampler_ = VK_NULL_HANDLE;
    }
}

void VulkanHdrTonemapPass::DestroyTonemapPipeline(VkDevice device) {
    if (tonemapPipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, tonemapPipeline_, nullptr);
        tonemapPipeline_ = VK_NULL_HANDLE;
    }
    if (tonemapPipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, tonemapPipelineLayout_, nullptr);
        tonemapPipelineLayout_ = VK_NULL_HANDLE;
    }
    if (tonemapDescriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, tonemapDescriptorPool_, nullptr);
        tonemapDescriptorPool_ = VK_NULL_HANDLE;
    }
    if (tonemapDescriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, tonemapDescriptorSetLayout_, nullptr);
        tonemapDescriptorSetLayout_ = VK_NULL_HANDLE;
    }
    tonemapDescriptorSets_.Clear();
    if (tonemapVertModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, tonemapVertModule_, nullptr);
        tonemapVertModule_ = VK_NULL_HANDLE;
    }
    if (tonemapFragModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, tonemapFragModule_, nullptr);
        tonemapFragModule_ = VK_NULL_HANDLE;
    }
    if (tonemapVertexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, tonemapVertexBuffer_, nullptr);
        tonemapVertexBuffer_ = VK_NULL_HANDLE;
    }
    if (tonemapVertexMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, tonemapVertexMemory_, nullptr);
        tonemapVertexMemory_ = VK_NULL_HANDLE;
    }
}

void VulkanHdrTonemapPass::CreateTonemapPipeline(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkRenderPass presentRenderPass,
        std::uint32_t framesInFlight,
        const VulkanSpvShaderLoader& shaders) {
    DestroyTonemapPipeline(device);
    if (hdrRenderPass_ == VK_NULL_HANDLE || presentRenderPass == VK_NULL_HANDLE || flights_.IsEmpty()) {
        return;
    }

    const float triVerts[] = {-1.0F, -1.0F, 3.0F, -1.0F, -1.0F, 3.0F};
    constexpr VkDeviceSize triBytes = sizeof(triVerts);
    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            triBytes,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            tonemapVertexBuffer_,
            tonemapVertexMemory_);
    void* mapped = nullptr;
    if (vkMapMemory(device, tonemapVertexMemory_, 0, triBytes, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("vkMapMemory tonemap tri failed");
    }
    std::memcpy(mapped, triVerts, sizeof(triVerts));
    vkUnmapMemory(device, tonemapVertexMemory_);

    VkDescriptorSetLayoutBinding sampBinding{};
    sampBinding.binding = 0;
    sampBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampBinding.descriptorCount = 1;
    sampBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dslInfo{};
    dslInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslInfo.bindingCount = 1;
    dslInfo.pBindings = &sampBinding;
    if (vkCreateDescriptorSetLayout(device, &dslInfo, nullptr, &tonemapDescriptorSetLayout_) != VK_SUCCESS) {
        throw std::runtime_error("tonemap descriptor set layout failed");
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = framesInFlight;
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = framesInFlight;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &tonemapDescriptorPool_) != VK_SUCCESS) {
        throw std::runtime_error("tonemap descriptor pool failed");
    }

    tonemapDescriptorSets_.Resize(framesInFlight);
    for (std::uint32_t fi = 0; fi < framesInFlight; ++fi) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = tonemapDescriptorPool_;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &tonemapDescriptorSetLayout_;
        if (vkAllocateDescriptorSets(device, &allocInfo, &tonemapDescriptorSets_[fi]) != VK_SUCCESS) {
            throw std::runtime_error("tonemap descriptor set alloc failed");
        }
        UpdateTonemapDescriptor(device, fi);
    }

    tonemapVertModule_ = shaders.CreateShaderModule(shaders.ReadSpvFile("tonemap.vert.spv"));
    tonemapFragModule_ = shaders.CreateShaderModule(shaders.ReadSpvFile("tonemap.frag.spv"));

    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset = 0;
    pcRange.size = sizeof(TonemapPushConstants);

    VkPipelineLayoutCreateInfo plInfo{};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &tonemapDescriptorSetLayout_;
    plInfo.pushConstantRangeCount = 1;
    plInfo.pPushConstantRanges = &pcRange;
    if (vkCreatePipelineLayout(device, &plInfo, nullptr, &tonemapPipelineLayout_) != VK_SUCCESS) {
        throw std::runtime_error("tonemap pipeline layout failed");
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = tonemapVertModule_;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = tonemapFragModule_;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(float) * 2;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription attr{};
    attr.binding = 0;
    attr.location = 0;
    attr.format = VK_FORMAT_R32G32_SFLOAT;
    attr.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 1;
    vertexInput.pVertexAttributeDescriptions = &attr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0F;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT;
    blendAtt.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAtt;

    VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynStates;

    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeInfo.stageCount = 2;
    pipeInfo.pStages = stages;
    pipeInfo.pVertexInputState = &vertexInput;
    pipeInfo.pInputAssemblyState = &inputAssembly;
    pipeInfo.pViewportState = &viewportState;
    pipeInfo.pRasterizationState = &raster;
    pipeInfo.pMultisampleState = &ms;
    pipeInfo.pColorBlendState = &blend;
    pipeInfo.pDynamicState = &dynamicState;
    pipeInfo.layout = tonemapPipelineLayout_;
    pipeInfo.renderPass = presentRenderPass;
    pipeInfo.subpass = 0;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &tonemapPipeline_) != VK_SUCCESS) {
        throw std::runtime_error("tonemap pipeline create failed");
    }
}

void VulkanHdrTonemapPass::UpdateTonemapDescriptor(
        VkDevice device,
        std::uint32_t flightIndex,
        VkImageView colorViewOverride) {
    if (flightIndex >= tonemapDescriptorSets_.GetSize() || flightIndex >= flights_.GetSize() ||
        colorSampler_ == VK_NULL_HANDLE) {
        return;
    }
    const FlightTarget& hdr = flights_[flightIndex];
    const VkImageView colorView = (colorViewOverride != VK_NULL_HANDLE) ? colorViewOverride : hdr.colorView;
    if (colorView == VK_NULL_HANDLE) {
        return;
    }
    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.imageView = colorView;
    imgInfo.sampler = colorSampler_;
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = tonemapDescriptorSets_[flightIndex];
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imgInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void VulkanHdrTonemapPass::BeginColorAttachmentBarrierIfNeeded(
        VkCommandBuffer commandBuffer,
        std::uint32_t frameIndex) {
    if (!HasFlight(frameIndex)) {
        return;
    }
    FlightTarget& hdrFlight = flights_[frameIndex];
    if (hdrFlight.colorImage == VK_NULL_HANDLE ||
        hdrFlight.colorLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        return;
    }

    VkImageMemoryBarrier hdrBarrier{};
    hdrBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    hdrBarrier.srcAccessMask =
            (hdrFlight.colorLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ? VK_ACCESS_SHADER_READ_BIT : 0;
    hdrBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    hdrBarrier.oldLayout = hdrFlight.colorLayout;
    hdrBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    hdrBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hdrBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hdrBarrier.image = hdrFlight.colorImage;
    hdrBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    hdrBarrier.subresourceRange.levelCount = 1;
    hdrBarrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &hdrBarrier);
    hdrFlight.colorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void VulkanHdrTonemapPass::TransitionColorToShaderRead(VkCommandBuffer commandBuffer, std::uint32_t frameIndex) {
    if (!HasFlight(frameIndex)) {
        return;
    }
    FlightTarget& hdrFlight = flights_[frameIndex];
    hdrFlight.colorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkImageMemoryBarrier hdrToTonemap{};
    hdrToTonemap.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    hdrToTonemap.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    hdrToTonemap.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    hdrToTonemap.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    hdrToTonemap.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    hdrToTonemap.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hdrToTonemap.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hdrToTonemap.image = hdrFlight.colorImage;
    hdrToTonemap.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    hdrToTonemap.subresourceRange.levelCount = 1;
    hdrToTonemap.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &hdrToTonemap);
}

void VulkanHdrTonemapPass::RecordTonemap(
        VkCommandBuffer commandBuffer,
        std::uint32_t imageIndex,
        std::uint32_t flightIndex,
        VkRenderPass presentRenderPass,
        VkExtent2D swapchainExtent,
        const VulkanPresentationFramebuffers& presentationFramebuffers,
        float exposure) {
    if (tonemapPipeline_ == VK_NULL_HANDLE || flightIndex >= flights_.GetSize() ||
        flightIndex >= tonemapDescriptorSets_.GetSize() || flights_[flightIndex].framebuffer == VK_NULL_HANDLE ||
        imageIndex >= presentationFramebuffers.buffers.GetSize()) {
        return;
    }

    VkClearValue clear{};
    clear.color.float32[0] = 0.05F;
    clear.color.float32[1] = 0.06F;
    clear.color.float32[2] = 0.09F;
    clear.color.float32[3] = 1.0F;

    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = presentRenderPass;
    rpBegin.framebuffer = presentationFramebuffers.buffers[imageIndex];
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent = swapchainExtent;
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clear;

    vkCmdBeginRenderPass(commandBuffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tonemapPipeline_);

    VkViewport vp{};
    vp.x = 0.0F;
    vp.y = 0.0F;
    vp.width = static_cast<float>(swapchainExtent.width);
    vp.height = static_cast<float>(swapchainExtent.height);
    vp.minDepth = 0.0F;
    vp.maxDepth = 1.0F;
    vkCmdSetViewport(commandBuffer, 0, 1, &vp);
    VkRect2D sc{};
    sc.offset = {0, 0};
    sc.extent = swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &sc);

    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            tonemapPipelineLayout_,
            0,
            1,
            &tonemapDescriptorSets_[flightIndex],
            0,
            nullptr);

    TonemapPushConstants push{};
    push.exposure = exposure;
    push.invGamma = 1.0F / 2.35F;
    vkCmdPushConstants(
            commandBuffer,
            tonemapPipelineLayout_,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(TonemapPushConstants),
            &push);

    const VkDeviceSize vbOff = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &tonemapVertexBuffer_, &vbOff);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    // UI passes recorded by caller before vkCmdEndRenderPass.
}

}  // namespace Spark
