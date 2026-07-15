#include "spark/render/VulkanSceneOpaquePass.hpp"

#include "spark/core/Array.hpp"
#include "spark/render/VulkanSceneMeshDraw.hpp"
#include "spark/render/VulkanSceneRaster.hpp"
#include "spark/render/SceneLightingProfile.hpp"
#include "spark/render/VulkanScreenUiClip.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace Spark {

static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, emissiveFactor) == 176);
static_assert(sizeof(VulkanSceneOpaquePass::ModelPushConstants) == 192);

void VulkanSceneOpaquePass::Record(
        const VkCommandBuffer commandBuffer,
        const VulkanSceneOpaqueRecordContext& ctx) const {
    if (!ctx.sceneParamsValid || ctx.scene == nullptr || ctx.pipelineLit == VK_NULL_HANDLE ||
        ctx.pipelineLayout == VK_NULL_HANDLE || ctx.descriptorSet == VK_NULL_HANDLE) {
        return;
    }
    if (ctx.meshBindings.staticVertexBuffer == VK_NULL_HANDLE || ctx.meshBindings.staticIndexBuffer == VK_NULL_HANDLE) {
        return;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineLit);

    VkViewport viewport{};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width = static_cast<float>(ctx.extent.width);
    viewport.height = static_cast<float>(ctx.extent.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, ctx.scene, ctx.extent, fullScissor);

    const VkDeviceSize vbOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ctx.meshBindings.staticVertexBuffer, &vbOffset);
    vkCmdBindIndexBuffer(commandBuffer, ctx.meshBindings.staticIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            ctx.pipelineLayout,
            0,
            1,
            &ctx.descriptorSet,
            0,
            nullptr);

    bool skyPipelineBound = false;
    ModelPushConstants push{};
    SceneMeshGeometryBinding bound = SceneMeshGeometryBinding::None;

    auto applyLitRasterState = [&](const SceneDrawItem& item) {
        if (item.doubleSided || item.mesh == SceneMeshSlot::GroundPlane) {
            vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);
            return;
        }
        vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_BACK_BIT);
        vkCmdSetFrontFace(commandBuffer, VulkanSceneFrontFaceForModel(item.model));
    };

    for (std::size_t di = 0; di < ctx.scene->draws.GetSize(); ++di) {
        const SceneDrawItem& d = ctx.scene->draws[di];
        const bool isSky = (d.skyMode != SceneSkyMode::None);
        if (isSky) {
            if (!skyPipelineBound && ctx.pipelineSky != VK_NULL_HANDLE) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineSky);
                skyPipelineBound = true;
            }
        } else {
            if (skyPipelineBound) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineLit);
                skyPipelineBound = false;
            }
            applyLitRasterState(d);
        }

        const SceneMeshDrawRange range = ResolveSceneMeshDrawRange(d, di, ctx.meshBindings);
        if (!range.drawable) {
            continue;
        }

        if (range.binding == SceneMeshGeometryBinding::StaticScene) {
            if (bound != SceneMeshGeometryBinding::StaticScene) {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ctx.meshBindings.staticVertexBuffer, &vbOffset);
                vkCmdBindIndexBuffer(commandBuffer, ctx.meshBindings.staticIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                bound = SceneMeshGeometryBinding::StaticScene;
            }
        } else if (range.binding == SceneMeshGeometryBinding::CustomDynamic) {
            if (bound != SceneMeshGeometryBinding::CustomDynamic) {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ctx.meshBindings.customVertexBuffer, &vbOffset);
                vkCmdBindIndexBuffer(commandBuffer, ctx.meshBindings.customIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                bound = SceneMeshGeometryBinding::CustomDynamic;
            }
        }

        std::memcpy(push.model, d.model.m, sizeof(push.model));
        push.albedo[0] = d.albedo.x;
        push.albedo[1] = d.albedo.y;
        push.albedo[2] = d.albedo.z;
        push.albedo[3] = d.opacity > 0.0F ? d.opacity : 1.0F;
        push.textureLayer = d.textureLayer;
        push.skyMode = static_cast<std::int32_t>(d.skyMode);
        push.metallic = d.metallic;
        push.roughness = d.roughness;
        push.metallicFactor = d.metallicFactor;
        push.roughnessFactor = d.roughnessFactor;
        push.occlusionStrength = d.occlusionStrength;
        push.shadowFlags = d.shadowFlags;
        push.emissive[0] = d.emissiveColor.x;
        push.emissive[1] = d.emissiveColor.y;
        push.emissive[2] = d.emissiveColor.z;
        push.emissive[3] = d.emissiveIntensity;
        push.emissiveFactor[0] = d.emissiveFactor.x;
        push.emissiveFactor[1] = d.emissiveFactor.y;
        push.emissiveFactor[2] = d.emissiveFactor.z;
        push.emissiveFactor[3] = 0.0F;
        push.useSkinning = 0;
        push.jointCount = 0;
        push.shadingModel = static_cast<std::int32_t>(d.shadingModel);
        push.toonDiffuseBands = std::clamp(d.toonDiffuseBands, 2, 8);
        push.toonRimIntensity = d.toonRimIntensity;
        push.toonRimPower = d.toonRimPower;
        push.normalMapLayer = d.normalMapLayer;
        push.metallicRoughnessMapLayer = d.metallicRoughnessMapLayer;
        push.emissiveMapLayer = d.emissiveMapLayer;

        if (!d.jointPalette.IsEmpty() && d.skinnedMesh && ctx.skinSsboMapped != nullptr &&
            ctx.frameIndex < ctx.skinSsboMapped->GetSize() &&
            (*ctx.skinSsboMapped)[ctx.frameIndex] != nullptr) {
            const std::uint32_t jc = static_cast<std::uint32_t>(d.jointPalette.GetSize());
            if (jc > 0 && jc <= ctx.maxSkinJoints) {
                push.useSkinning = 1;
                push.jointCount = static_cast<std::int32_t>(jc);
                std::memcpy(
                        (*ctx.skinSsboMapped)[ctx.frameIndex],
                        d.jointPalette.GetData(),
                        static_cast<std::size_t>(jc) * sizeof(Matrix4));
            }
        }

        vkCmdPushConstants(
                commandBuffer,
                ctx.pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(ModelPushConstants),
                &push);
        vkCmdDrawIndexed(commandBuffer, range.indexCount, 1, range.firstIndex, range.vertexOffset, 0);
    }

    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullScissor);
}

void VulkanSceneOpaquePass::RecordTransparent(
        const VkCommandBuffer commandBuffer,
        const VulkanSceneOpaqueRecordContext& ctx,
        const Array<CustomMeshGpuSlice>& transparentPacked) const {
    if (!ctx.sceneParamsValid || ctx.scene == nullptr || ctx.pipelineLitTransparent == VK_NULL_HANDLE ||
        ctx.pipelineLayout == VK_NULL_HANDLE || ctx.descriptorSet == VK_NULL_HANDLE) {
        return;
    }
    if (ctx.scene->transparentDraws.IsEmpty()) {
        return;
    }
    if (ctx.meshBindings.staticVertexBuffer == VK_NULL_HANDLE || ctx.meshBindings.staticIndexBuffer == VK_NULL_HANDLE) {
        return;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineLitTransparent);

    const VkDeviceSize vbOffset = 0;
    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            ctx.pipelineLayout,
            0,
            1,
            &ctx.descriptorSet,
            0,
            nullptr);

    SceneMeshDrawBindings transparentBindings = ctx.meshBindings;
    transparentBindings.customDrawPacked = &transparentPacked;

    ModelPushConstants push{};
    SceneMeshGeometryBinding bound = SceneMeshGeometryBinding::None;

    auto applyLitRasterState = [&](const SceneDrawItem& item) {
        if (item.doubleSided || item.mesh == SceneMeshSlot::GroundPlane) {
            vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);
            return;
        }
        vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_BACK_BIT);
        vkCmdSetFrontFace(commandBuffer, VulkanSceneFrontFaceForModel(item.model));
    };

    for (std::size_t di = 0; di < ctx.scene->transparentDraws.GetSize(); ++di) {
        const SceneDrawItem& d = ctx.scene->transparentDraws[di];
        if (d.skyMode != SceneSkyMode::None) {
            continue;
        }
        applyLitRasterState(d);

        const SceneMeshDrawRange range = ResolveSceneMeshDrawRange(d, di, transparentBindings);
        if (!range.drawable) {
            continue;
        }

        if (range.binding == SceneMeshGeometryBinding::StaticScene) {
            if (bound != SceneMeshGeometryBinding::StaticScene) {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &transparentBindings.staticVertexBuffer, &vbOffset);
                vkCmdBindIndexBuffer(commandBuffer, transparentBindings.staticIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                bound = SceneMeshGeometryBinding::StaticScene;
            }
        } else if (range.binding == SceneMeshGeometryBinding::CustomDynamic) {
            if (bound != SceneMeshGeometryBinding::CustomDynamic) {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &transparentBindings.customVertexBuffer, &vbOffset);
                vkCmdBindIndexBuffer(commandBuffer, transparentBindings.customIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                bound = SceneMeshGeometryBinding::CustomDynamic;
            }
        }

        std::memcpy(push.model, d.model.m, sizeof(push.model));
        push.albedo[0] = d.albedo.x;
        push.albedo[1] = d.albedo.y;
        push.albedo[2] = d.albedo.z;
        push.albedo[3] = d.opacity > 0.0F ? d.opacity : 1.0F;
        push.textureLayer = d.textureLayer;
        push.skyMode = 0;
        push.metallic = d.metallic;
        push.roughness = d.roughness;
        push.metallicFactor = d.metallicFactor;
        push.roughnessFactor = d.roughnessFactor;
        push.occlusionStrength = d.occlusionStrength;
        push.shadowFlags = d.shadowFlags;
        push.emissive[0] = d.emissiveColor.x;
        push.emissive[1] = d.emissiveColor.y;
        push.emissive[2] = d.emissiveColor.z;
        push.emissive[3] = d.emissiveIntensity;
        push.emissiveFactor[0] = d.emissiveFactor.x;
        push.emissiveFactor[1] = d.emissiveFactor.y;
        push.emissiveFactor[2] = d.emissiveFactor.z;
        push.emissiveFactor[3] = 0.0F;
        push.useSkinning = 0;
        push.jointCount = 0;
        push.shadingModel = static_cast<std::int32_t>(d.shadingModel);
        push.toonDiffuseBands = std::clamp(d.toonDiffuseBands, 2, 8);
        push.toonRimIntensity = d.toonRimIntensity;
        push.toonRimPower = d.toonRimPower;
        push.normalMapLayer = d.normalMapLayer;
        push.metallicRoughnessMapLayer = d.metallicRoughnessMapLayer;
        push.emissiveMapLayer = d.emissiveMapLayer;

        if (!d.jointPalette.IsEmpty() && d.skinnedMesh && ctx.skinSsboMapped != nullptr &&
            ctx.frameIndex < ctx.skinSsboMapped->GetSize() &&
            (*ctx.skinSsboMapped)[ctx.frameIndex] != nullptr) {
            const std::uint32_t jc = static_cast<std::uint32_t>(d.jointPalette.GetSize());
            if (jc > 0 && jc <= ctx.maxSkinJoints) {
                push.useSkinning = 1;
                push.jointCount = static_cast<std::int32_t>(jc);
                std::memcpy(
                        (*ctx.skinSsboMapped)[ctx.frameIndex],
                        d.jointPalette.GetData(),
                        static_cast<std::size_t>(jc) * sizeof(Matrix4));
            }
        }

        vkCmdPushConstants(
                commandBuffer,
                ctx.pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(ModelPushConstants),
                &push);
        vkCmdDrawIndexed(commandBuffer, range.indexCount, 1, range.firstIndex, range.vertexOffset, 0);
    }
}

}  // namespace Spark
