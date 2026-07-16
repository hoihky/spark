#include "spark/render/VulkanParticlePass.hpp"

#include "spark/render/VulkanRendererGpu.hpp"
#include "spark/render/VulkanScreenUiClip.hpp"

#include <cstring>
#include <stdexcept>

namespace Spark {

namespace {

constexpr VkDeviceSize kParticleUboBytes = 256;
constexpr VkDeviceSize kParticleVertexBytes = 10 * 1024 * 1024;
constexpr std::uint32_t kParticleFloatsPerVertex = 10;

}  // namespace

void VulkanParticlePass::CreateGpuResources(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const std::uint32_t framesInFlight,
        const VulkanSpvShaderLoader& shaders) {
    if (device == VK_NULL_HANDLE) {
        return;
    }

    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo dslInfo{};
    dslInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslInfo.bindingCount = 1;
    dslInfo.pBindings = &uboBinding;
    if (vkCreateDescriptorSetLayout(device, &dslInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorSetLayout (particle) failed");
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = framesInFlight;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = framesInFlight;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorPool (particle) failed");
    }

    Array<VkDescriptorSetLayout> layouts;
    layouts.Resize(framesInFlight);
    for (std::size_t i = 0; i < framesInFlight; ++i) {
        layouts[i] = descriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = framesInFlight;
    allocInfo.pSetLayouts = layouts.GetData();
    descriptorSets.Resize(framesInFlight);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.GetData()) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateDescriptorSets (particle) failed");
    }

    uniformBuffers.Resize(framesInFlight);
    uniformBuffersMemory.Resize(framesInFlight);
    uniformBuffersMapped.Resize(framesInFlight);
    for (std::size_t i = 0; i < framesInFlight; ++i) {
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                kParticleUboBytes,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[i],
                uniformBuffersMemory[i]);
        if (vkMapMemory(
                    device,
                    uniformBuffersMemory[i],
                    0,
                    kParticleUboBytes,
                    0,
                    &uniformBuffersMapped[i]) != VK_SUCCESS) {
            throw std::runtime_error("map particle UBO failed");
        }
        VkDescriptorBufferInfo bi{};
        bi.buffer = uniformBuffers[i];
        bi.offset = 0;
        bi.range = kParticleUboBytes;
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bi;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            kParticleVertexBytes,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertexBuffer,
            vertexBufferMemory);
    if (vkMapMemory(device, vertexBufferMemory, 0, kParticleVertexBytes, 0, &vertexMapped) != VK_SUCCESS) {
        throw std::runtime_error("map particle vertex buffer failed");
    }
    vertexCapacityBytes = kParticleVertexBytes;

    const Array<char> pv = shaders.ReadSpvFile("particle.vert.spv");
    const Array<char> pf = shaders.ReadSpvFile("particle.frag.spv");
    vertModule = shaders.CreateShaderModule(pv);
    fragModule = shaders.CreateShaderModule(pf);
}

void VulkanParticlePass::DestroyGpuResources(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (vertexMapped != nullptr) {
        vkUnmapMemory(device, vertexBufferMemory);
        vertexMapped = nullptr;
    }
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
    vertexCapacityBytes = 0;

    for (std::size_t i = 0; i < uniformBuffers.GetSize(); ++i) {
        if (uniformBuffersMapped[i] != nullptr) {
            vkUnmapMemory(device, uniformBuffersMemory[i]);
            uniformBuffersMapped[i] = nullptr;
        }
        if (uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        }
        if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
    }
    uniformBuffers.Clear();
    uniformBuffersMemory.Clear();
    uniformBuffersMapped.Clear();

    descriptorSets.Clear();
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (vertModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vertModule, nullptr);
        vertModule = VK_NULL_HANDLE;
    }
    if (fragModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, fragModule, nullptr);
        fragModule = VK_NULL_HANDLE;
    }
}

void VulkanParticlePass::DestroyGraphicsPipeline(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
}

void VulkanParticlePass::CreateGraphicsPipeline(const VkDevice device, const VkRenderPass hdrRenderPass) {
    if (device == VK_NULL_HANDLE || hdrRenderPass == VK_NULL_HANDLE || vertModule == VK_NULL_HANDLE ||
        fragModule == VK_NULL_HANDLE || descriptorSetLayout == VK_NULL_HANDLE) {
        return;
    }
    DestroyGraphicsPipeline(device);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    const VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    constexpr std::uint32_t kStride = sizeof(float) * kParticleFloatsPerVertex;
    VkVertexInputBindingDescription bind{};
    bind.binding = 0;
    bind.stride = kStride;
    bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[4]{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = 0;
    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[1].offset = sizeof(float) * 3;
    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32_SFLOAT;
    attrs[2].offset = sizeof(float) * 7;
    attrs[3].binding = 0;
    attrs[3].location = 3;
    attrs[3].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[3].offset = sizeof(float) * 8;

    VkPipelineVertexInputStateCreateInfo vtxIn{};
    vtxIn.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vtxIn.vertexBindingDescriptionCount = 1;
    vtxIn.pVertexBindingDescriptions = &bind;
    vtxIn.vertexAttributeDescriptionCount = 4;
    vtxIn.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rast{};
    rast.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.polygonMode = VK_POLYGON_MODE_FILL;
    rast.lineWidth = 1.0F;
    rast.cullMode = VK_CULL_MODE_NONE;
    rast.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_FALSE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
    blendAtt.blendEnable = VK_TRUE;
    blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAtt.colorBlendOp = VK_BLEND_OP_ADD;
    blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAtt.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAtt;

    const VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2u;
    dyn.pDynamicStates = dynStates;

    VkPipelineLayoutCreateInfo pl{};
    pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &descriptorSetLayout;
    pl.pushConstantRangeCount = 0;
    if (vkCreatePipelineLayout(device, &pl, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("vkCreatePipelineLayout (particle) failed");
    }

    VkGraphicsPipelineCreateInfo pipe{};
    pipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipe.stageCount = 2;
    pipe.pStages = stages;
    pipe.pVertexInputState = &vtxIn;
    pipe.pInputAssemblyState = &ia;
    pipe.pViewportState = &vp;
    pipe.pRasterizationState = &rast;
    pipe.pMultisampleState = &ms;
    pipe.pDepthStencilState = &ds;
    pipe.pColorBlendState = &blend;
    pipe.pDynamicState = &dyn;
    pipe.layout = pipelineLayout;
    pipe.renderPass = hdrRenderPass;
    pipe.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateGraphicsPipelines (particle) failed");
    }
}

void VulkanParticlePass::Record(
        const VkCommandBuffer commandBuffer,
        const std::uint32_t frameIndex,
        const VkExtent2D extent,
        const SceneRenderParams& scene,
        const bool sceneParamsValid) const {
    if (!sceneParamsValid || pipeline == VK_NULL_HANDLE || vertexMapped == nullptr ||
        frameIndex >= descriptorSets.GetSize() || scene.particles.IsEmpty()) {
        return;
    }

    const std::size_t maxByBuffer =
            static_cast<std::size_t>(vertexCapacityBytes / (sizeof(float) * kParticleFloatsPerVertex)) / 4U;
    std::size_t nPart = scene.particles.GetSize();
    if (nPart > static_cast<std::size_t>(SceneRenderParams::MaxParticles)) {
        nPart = static_cast<std::size_t>(SceneRenderParams::MaxParticles);
    }
    if (nPart > maxByBuffer) {
        nPart = maxByBuffer;
    }
    if (nPart == 0) {
        return;
    }

    ParticleUniformGpu ubo{};
    std::memcpy(ubo.viewProj, scene.viewProjection.m, sizeof(ubo.viewProj));
    ubo.cameraPos[0] = scene.cameraPositionWorld.x;
    ubo.cameraPos[1] = scene.cameraPositionWorld.y;
    ubo.cameraPos[2] = scene.cameraPositionWorld.z;
    ubo.cameraPos[3] = 0.0F;
    ubo.cameraRight[0] = scene.particleCameraRight.x;
    ubo.cameraRight[1] = scene.particleCameraRight.y;
    ubo.cameraRight[2] = scene.particleCameraRight.z;
    ubo.cameraRight[3] = 0.0F;
    ubo.cameraUp[0] = scene.particleCameraUp.x;
    ubo.cameraUp[1] = scene.particleCameraUp.y;
    ubo.cameraUp[2] = scene.particleCameraUp.z;
    ubo.cameraUp[3] = 0.0F;
    std::memcpy(uniformBuffersMapped[frameIndex], &ubo, sizeof(ParticleUniformGpu));

    scratchVertices.Clear();
    const float corners[4][2] = {{-1.0F, -1.0F}, {1.0F, -1.0F}, {1.0F, 1.0F}, {-1.0F, 1.0F}};
    const std::uint32_t idx0[] = {0, 1, 2, 0, 2, 3};
    for (std::size_t pi = 0; pi < nPart; ++pi) {
        const SceneParticleInstance& p = scene.particles[pi];
        for (int t = 0; t < 6; ++t) {
            const int k = static_cast<int>(idx0[t]);
            scratchVertices.PushBack(p.position.x);
            scratchVertices.PushBack(p.position.y);
            scratchVertices.PushBack(p.position.z);
            scratchVertices.PushBack(p.color.x);
            scratchVertices.PushBack(p.color.y);
            scratchVertices.PushBack(p.color.z);
            scratchVertices.PushBack(p.color.w);
            scratchVertices.PushBack(p.size);
            scratchVertices.PushBack(corners[k][0]);
            scratchVertices.PushBack(corners[k][1]);
        }
    }

    const VkDeviceSize vbBytes = sizeof(float) * static_cast<VkDeviceSize>(scratchVertices.GetSize());
    std::memcpy(vertexMapped, scratchVertices.GetData(), static_cast<std::size_t>(vbBytes));

    VkViewport viewport{};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, &scene, extent, fullScissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &descriptorSets[frameIndex],
            0,
            nullptr);

    const VkDeviceSize vbOff = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vbOff);

    const std::uint32_t vertCount = static_cast<std::uint32_t>(scratchVertices.GetSize() / kParticleFloatsPerVertex);
    vkCmdDraw(commandBuffer, vertCount, 1, 0, 0);

    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullScissor);
}

}  // namespace Spark
