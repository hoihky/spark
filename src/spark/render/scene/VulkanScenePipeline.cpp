#include "spark/render/scene/VulkanScenePipeline.hpp"

#include "spark/core/Array.hpp"
#include "spark/render/scene/VulkanSceneOpaquePass.hpp"
#include "spark/render/scene/VulkanSceneVertexLayout.hpp"

#include <stdexcept>

namespace Spark {

void VulkanScenePipeline::DestroyGraphicsPipeline(const VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (pipelineLit != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipelineLit, nullptr);
        pipelineLit = VK_NULL_HANDLE;
    }
    if (pipelineLitTransparent != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipelineLitTransparent, nullptr);
        pipelineLitTransparent = VK_NULL_HANDLE;
    }
    if (pipelineSky != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipelineSky, nullptr);
        pipelineSky = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
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

void VulkanScenePipeline::CreateGraphicsPipeline(
        const VkDevice device,
        const VkRenderPass hdrRenderPass,
        const VkDescriptorSetLayout sceneDescriptorSetLayout,
        const VulkanSpvShaderLoader& shaders) {
    if (device == VK_NULL_HANDLE || hdrRenderPass == VK_NULL_HANDLE ||
        sceneDescriptorSetLayout == VK_NULL_HANDLE) {
        return;
    }
    DestroyGraphicsPipeline(device);

    const Array<char> vertCode = shaders.ReadSpvFile("scene.vert.spv");
    const Array<char> fragCode = shaders.ReadSpvFile("scene.frag.spv");
    vertModule = shaders.CreateShaderModule(vertCode);
    fragModule = shaders.CreateShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo vertShaderStage{};
    vertShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStage.module = vertModule;
    vertShaderStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStage{};
    fragShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStage.module = fragModule;
    fragShaderStage.pName = "main";

    const VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStage, fragShaderStage};

    using VL = VulkanSceneVertexLayout;
    constexpr std::uint32_t stride = VL::kStrideBytes;
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = stride;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[6]{};
    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = sizeof(float) * VL::kOffPosition;
    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = sizeof(float) * VL::kOffNormal;
    attrs[2].binding = 0;
    attrs[2].location = 2;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = sizeof(float) * VL::kOffTexCoord;
    attrs[3].binding = 0;
    attrs[3].location = 3;
    attrs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[3].offset = sizeof(float) * VL::kOffTangent;
    attrs[4].binding = 0;
    attrs[4].location = 4;
    attrs[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[4].offset = sizeof(float) * VL::kOffJoints;
    attrs[5].binding = 0;
    attrs[5].location = 5;
    attrs[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[5].offset = sizeof(float) * VL::kOffWeights;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &binding;
    vertexInputInfo.vertexAttributeDescriptionCount = 6;
    vertexInputInfo.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0F;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineRasterizationStateCreateInfo rasterizerSky{};
    rasterizerSky.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerSky.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerSky.lineWidth = 1.0F;
    rasterizerSky.cullMode = VK_CULL_MODE_NONE;
    rasterizerSky.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilSky{};
    depthStencilSky.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilSky.depthTestEnable = VK_FALSE;
    depthStencilSky.depthWriteEnable = VK_FALSE;
    depthStencilSky.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilSky.depthBoundsTestEnable = VK_FALSE;
    depthStencilSky.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState transparentBlendAttachment = colorBlendAttachment;
    transparentBlendAttachment.blendEnable = VK_TRUE;
    transparentBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    transparentBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    transparentBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    transparentBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    transparentBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    transparentBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineColorBlendStateCreateInfo transparentColorBlending = colorBlending;
    transparentColorBlending.pAttachments = &transparentBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencilTransparent = depthStencil;
    depthStencilTransparent.depthWriteEnable = VK_FALSE;

    Array<VkDynamicState> dynamicStatesList;
    dynamicStatesList.PushBack(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStatesList.PushBack(VK_DYNAMIC_STATE_SCISSOR);
    dynamicStatesList.PushBack(VK_DYNAMIC_STATE_CULL_MODE);
    dynamicStatesList.PushBack(VK_DYNAMIC_STATE_FRONT_FACE);
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStatesList.GetSize());
    dynamicState.pDynamicStates = dynamicStatesList.GetData();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(VulkanSceneOpaquePass::ModelPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &sceneDescriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("VulkanScenePipeline: vkCreatePipelineLayout failed");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState = &multisampling;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
    pipelineCreateInfo.pColorBlendState = &colorBlending;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = hdrRenderPass;
    pipelineCreateInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineLit) !=
            VK_SUCCESS) {
        throw std::runtime_error("VulkanScenePipeline: vkCreateGraphicsPipelines failed");
    }

    pipelineCreateInfo.pDepthStencilState = &depthStencilSky;
    pipelineCreateInfo.pRasterizationState = &rasterizerSky;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineSky) !=
            VK_SUCCESS) {
        throw std::runtime_error("VulkanScenePipeline: vkCreateGraphicsPipelines (sky) failed");
    }

    pipelineCreateInfo.pDepthStencilState = &depthStencilTransparent;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pColorBlendState = &transparentColorBlending;
    if (vkCreateGraphicsPipelines(
                device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineLitTransparent) !=
            VK_SUCCESS) {
        throw std::runtime_error("VulkanScenePipeline: vkCreateGraphicsPipelines (transparent) failed");
    }
}

}  // namespace Spark
