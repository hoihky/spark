#include "spark/render/scene/VulkanWaterPass.hpp"

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/gpu/VulkanSpvShaderLoader.hpp"
#include "spark/render/scene/VulkanSceneRaster.hpp"
#include "spark/render/scene/VulkanSceneVertexLayout.hpp"
#include "spark/render/scene/VulkanWaterPushConstants.hpp"
#include "spark/render/scene/WaterPushConstantsBuilder.hpp"
#include "spark/render/ui/VulkanScreenUiClip.hpp"

#include <stdexcept>

namespace Spark {

void VulkanWaterPass::DestroyGraphicsPipeline(const VkDevice device) noexcept {
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
    if (vertModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vertModule, nullptr);
        vertModule = VK_NULL_HANDLE;
    }
    if (fragModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, fragModule, nullptr);
        fragModule = VK_NULL_HANDLE;
    }
}

void VulkanWaterPass::CreateGraphicsPipeline(
        const VkDevice device,
        const VkRenderPass hdrRenderPass,
        const VkDescriptorSetLayout sceneDescriptorSetLayout,
        const VulkanSpvShaderLoader& shaders) {
    if (device == VK_NULL_HANDLE || hdrRenderPass == VK_NULL_HANDLE ||
        sceneDescriptorSetLayout == VK_NULL_HANDLE) {
        return;
    }
    DestroyGraphicsPipeline(device);

    const Array<char> vertCode = shaders.ReadSpvFile("water.vert.spv");
    const Array<char> fragCode = shaders.ReadSpvFile("water.frag.spv");
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

    VkVertexInputAttributeDescription attrs[3]{};
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
    attrs[2].offset = sizeof(float) * VL::kOffTexCoord0;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &binding;
    vertexInputInfo.vertexAttributeDescriptionCount = 3;
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
    // Cull/front-face are set per draw via VulkanSceneApplyRasterState (dynamic state).
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    // LESS fails at the far plane against the cleared sky depth (1.0); sky does not write depth.
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

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
    pushConstantRange.size = sizeof(WaterPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &sceneDescriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("VulkanWaterPass: vkCreatePipelineLayout failed");
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
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("VulkanWaterPass: vkCreateGraphicsPipelines failed");
    }
}

void VulkanWaterPass::Record(
        const VkCommandBuffer commandBuffer,
        const VulkanWaterRecordContext& ctx,
        const Array<CustomMeshGpuSlice>& waterCustomPacked) const {
    if (!ctx.sceneParamsValid || ctx.scene == nullptr || ctx.pipeline == VK_NULL_HANDLE ||
        ctx.pipelineLayout == VK_NULL_HANDLE || ctx.descriptorSet == VK_NULL_HANDLE ||
        ctx.scene->waterDraws.IsEmpty()) {
        return;
    }
    if (ctx.meshBindings.staticVertexBuffer == VK_NULL_HANDLE || ctx.meshBindings.staticIndexBuffer == VK_NULL_HANDLE) {
        return;
    }

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, ctx.scene, ctx.extent, fullScissor);
    VulkanScreenUiClip::BindScenePassViewport(commandBuffer, ctx.scene, ctx.extent);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipeline);
    vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_BACK_BIT);
    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);
    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            ctx.pipelineLayout,
            0,
            1,
            &ctx.descriptorSet,
            0,
            nullptr);

    SceneMeshDrawBindings meshBindings = ctx.meshBindings;
    meshBindings.customDrawPacked = &waterCustomPacked;

    const VkDeviceSize vbOffset = 0;
    SceneMeshGeometryBinding bound = SceneMeshGeometryBinding::None;
    WaterPushConstants push{};

    for (std::size_t di = 0; di < ctx.scene->waterDraws.GetSize(); ++di) {
        const SceneWaterDraw& waterDraw = ctx.scene->waterDraws[di];
        const SceneDrawItem& d = waterDraw.item;
        const SceneMeshDrawRange range = ResolveSceneMeshDrawRange(d, di, meshBindings);
        if (!range.drawable) {
            continue;
        }

        if (range.binding == SceneMeshGeometryBinding::StaticScene) {
            if (bound != SceneMeshGeometryBinding::StaticScene) {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshBindings.staticVertexBuffer, &vbOffset);
                vkCmdBindIndexBuffer(commandBuffer, meshBindings.staticIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                bound = SceneMeshGeometryBinding::StaticScene;
            }
        } else if (range.binding == SceneMeshGeometryBinding::CustomDynamic) {
            if (bound != SceneMeshGeometryBinding::CustomDynamic) {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshBindings.customVertexBuffer, &vbOffset);
                vkCmdBindIndexBuffer(commandBuffer, meshBindings.customIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                bound = SceneMeshGeometryBinding::CustomDynamic;
            }
        }

        VulkanSceneApplyRasterState(commandBuffer, d, SceneSkyMode::None);

        push = WaterPushConstantsBuilder(waterDraw).Get();
        vkCmdPushConstants(
                commandBuffer,
                ctx.pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(WaterPushConstants),
                &push);
        vkCmdDrawIndexed(commandBuffer, range.indexCount, 1, range.firstIndex, range.vertexOffset, 0);
    }

    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullScissor);
}

}  // namespace Spark
