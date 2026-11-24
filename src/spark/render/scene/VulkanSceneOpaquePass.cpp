#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/VulkanSceneOpaquePass.hpp"

#include "spark/core/Array.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/scene/VulkanCustomMeshPool.hpp"
#include "spark/render/scene/VulkanRigidMeshInstanceGpu.hpp"
#include "spark/render/scene/VulkanSceneMeshDraw.hpp"
#include "spark/scene/mesh/MeshInstanceRecord.hpp"
#include "spark/scene/mesh/SceneMeshInstancedBatch.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/render/scene/VulkanSceneRaster.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/render/ui/VulkanScreenUiClip.hpp"
#include "spark/scene/material/MaterialGltfExtensions.hpp"
#include "spark/scene/material/MaterialUvMap.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace {

void WriteMapUvPush(
        Spark::VulkanSceneOpaquePass::ModelPushConstants& push,
        const int mapIndex,
        const Spark::MaterialUvMap& map) noexcept {
    push.mapUvScale[mapIndex][0] = map.uvScale.x;
    push.mapUvScale[mapIndex][1] = map.uvScale.y;
    push.mapUvOffset[mapIndex][0] = map.uvOffset.x;
    push.mapUvOffset[mapIndex][1] = map.uvOffset.y;
    push.mapUvRotation[mapIndex] = map.uvRotation;
    push.mapTexCoordSet[mapIndex] = static_cast<std::int32_t>(map.texCoordSet);
}

void FillMaterialMapUvPush(
        Spark::VulkanSceneOpaquePass::ModelPushConstants& push,
        const Spark::SceneDrawItem& draw) noexcept {
    if (draw.skyMode != Spark::SceneSkyMode::None) {
        Spark::MaterialUvMap equirectUv{};
        equirectUv.uvScale = draw.textureUvScale;
        WriteMapUvPush(push, 0, equirectUv);
    } else {
        Spark::MaterialUvMap base = draw.baseColorUv;
        base.uvScale.x *= draw.textureUvScale.x;
        base.uvScale.y *= draw.textureUvScale.y;
        base.uvOffset.x += draw.textureUvOffset.x;
        base.uvOffset.y += draw.textureUvOffset.y;
        WriteMapUvPush(push, 0, base);
    }
    WriteMapUvPush(push, 1, draw.normalUv);
    WriteMapUvPush(push, 2, draw.metallicRoughnessUv);
    WriteMapUvPush(push, 3, draw.emissiveUv);
    WriteMapUvPush(push, 4, draw.iridescenceThicknessUv);
}

void FillMaterialExtensionPush(
        Spark::VulkanSceneOpaquePass::ModelPushConstants& push,
        const Spark::MaterialGltfExtensions& ext) noexcept {
    push.clearcoatFactor = ext.clearcoatFactor;
    push.clearcoatRoughnessFactor = ext.clearcoatRoughnessFactor;
    push.transmissionFactor = ext.transmissionFactor;
    push.emissiveStrength = ext.emissiveStrength;
    push.iridescenceFactor = ext.iridescenceFactor;
    push.iridescenceIor = ext.iridescenceIor;
    push.iridescenceThicknessMin = ext.iridescenceThicknessMin;
    push.iridescenceThicknessMax = ext.iridescenceThicknessMax;
}

}  // namespace

namespace Spark {

static_assert(sizeof(VulkanSceneOpaquePass::ModelPushConstants) == 352);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, mapUvScale) == 176);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, clearcoatFactor) == 296);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, emissiveStrength) == 304);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, transmissionFactor) == 308);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, emissiveFactor) == 312);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, albedoHdrLinear) == 324);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, normalScale) == 328);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, iridescenceFactor) == 332);
static_assert(offsetof(VulkanSceneOpaquePass::ModelPushConstants, iridescenceThicknessMax) == 344);

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

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, ctx.scene, ctx.extent, fullScissor);
    VulkanScreenUiClip::BindScenePassViewport(commandBuffer, ctx.scene, ctx.extent);

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
        }

        const SceneMeshDrawRange range = ResolveSceneMeshDrawRange(d, di, ctx.meshBindings);
        if (!range.drawable) {
            continue;
        }

        VulkanSceneApplyRasterState(commandBuffer, d, d.skyMode);

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
        push.alphaCutoff = d.alphaCutoff;
        FillMaterialMapUvPush(push, d);
        push.emissive[0] = d.emissiveColor.x;
        push.emissive[1] = d.emissiveColor.y;
        push.emissive[2] = d.emissiveColor.z;
        push.emissive[3] = d.emissiveIntensity;
        push.emissiveFactor[0] = d.emissiveFactor.x;
        push.emissiveFactor[1] = d.emissiveFactor.y;
        push.emissiveFactor[2] = d.emissiveFactor.z;
        FillMaterialExtensionPush(push, d.gltfExtensions);
        push.albedoHdrLinear = d.textureIsHdrLinear ? 1 : 0;
        push.normalScale = d.normalScale;
        push.useSkinning = 0;
        push.jointCount = 0;
        push.shadingModel = static_cast<std::int32_t>(d.shadingModel);
        push.toonDiffuseBands = std::clamp(d.toonDiffuseBands, 2, 8);
        push.toonRimIntensity = d.toonRimIntensity;
        push.toonRimPower = d.toonRimPower;
        push.normalMapLayer = d.normalMapLayer;
        push.metallicRoughnessMapLayer = d.metallicRoughnessMapLayer;
        push.emissiveMapLayer = d.emissiveMapLayer;
        push.iridescenceThicknessMapLayer = d.iridescenceThicknessMapLayer;

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

        push.instanceBase = 0;
        vkCmdPushConstants(
                commandBuffer,
                ctx.pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(ModelPushConstants),
                &push);
        vkCmdDrawIndexed(commandBuffer, range.indexCount, 1, range.firstIndex, range.vertexOffset, 0);
    }

    RecordRigidInstancedBatches(commandBuffer, ctx);
    VulkanScreenUiClip::RestoreFramebufferScissor(commandBuffer, fullScissor);
}

void VulkanSceneOpaquePass::RecordLitMeshDraws(
        const VkCommandBuffer commandBuffer,
        const VulkanSceneOpaqueRecordContext& ctx,
        const Array<SceneDrawItem>& draws,
        const Array<CustomMeshGpuSlice>& customPacked) const {
    if (!ctx.sceneParamsValid || ctx.pipelineLit == VK_NULL_HANDLE || ctx.pipelineLayout == VK_NULL_HANDLE ||
        ctx.descriptorSet == VK_NULL_HANDLE || draws.IsEmpty()) {
        return;
    }
    if (ctx.meshBindings.staticVertexBuffer == VK_NULL_HANDLE || ctx.meshBindings.staticIndexBuffer == VK_NULL_HANDLE) {
        return;
    }

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, ctx.scene, ctx.extent, fullScissor);
    VulkanScreenUiClip::BindScenePassViewport(commandBuffer, ctx.scene, ctx.extent);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineLit);

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

    SceneMeshDrawBindings meshBindings = ctx.meshBindings;
    meshBindings.customDrawPacked = &customPacked;

    ModelPushConstants push{};
    SceneMeshGeometryBinding bound = SceneMeshGeometryBinding::None;

    for (std::size_t di = 0; di < draws.GetSize(); ++di) {
        const SceneDrawItem& d = draws[di];
        const SceneMeshDrawRange range = ResolveSceneMeshDrawRange(d, di, meshBindings);
        if (!range.drawable) {
            continue;
        }

        VulkanSceneApplyRasterState(commandBuffer, d, SceneSkyMode::None);

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

        std::memcpy(push.model, d.model.m, sizeof(push.model));
        push.albedo[0] = d.albedo.x;
        push.albedo[1] = d.albedo.y;
        push.albedo[2] = d.albedo.z;
        push.albedo[3] = d.opacity > 0.0F ? d.opacity : 1.0F;
        push.textureLayer = d.textureLayer;
        push.skyMode = static_cast<std::int32_t>(SceneSkyMode::None);
        push.metallic = d.metallic;
        push.roughness = d.roughness;
        push.metallicFactor = d.metallicFactor;
        push.roughnessFactor = d.roughnessFactor;
        push.occlusionStrength = d.occlusionStrength;
        push.shadowFlags = d.shadowFlags;
        push.alphaCutoff = d.alphaCutoff;
        FillMaterialMapUvPush(push, d);
        push.emissive[0] = d.emissiveColor.x;
        push.emissive[1] = d.emissiveColor.y;
        push.emissive[2] = d.emissiveColor.z;
        push.emissive[3] = d.emissiveIntensity;
        push.emissiveFactor[0] = d.emissiveFactor.x;
        push.emissiveFactor[1] = d.emissiveFactor.y;
        push.emissiveFactor[2] = d.emissiveFactor.z;
        FillMaterialExtensionPush(push, d.gltfExtensions);
        push.albedoHdrLinear = d.textureIsHdrLinear ? 1 : 0;
        push.normalScale = d.normalScale;
        push.useSkinning = 0;
        push.jointCount = 0;
        push.shadingModel = static_cast<std::int32_t>(d.shadingModel);
        push.toonDiffuseBands = std::clamp(d.toonDiffuseBands, 2, 8);
        push.toonRimIntensity = d.toonRimIntensity;
        push.toonRimPower = d.toonRimPower;
        push.normalMapLayer = d.normalMapLayer;
        push.metallicRoughnessMapLayer = d.metallicRoughnessMapLayer;
        push.emissiveMapLayer = d.emissiveMapLayer;
        push.iridescenceThicknessMapLayer = d.iridescenceThicknessMapLayer;

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

    VkRect2D fullScissor{};
    VulkanScreenUiClip::BindScenePassScissor(commandBuffer, ctx.scene, ctx.extent, fullScissor);
    VulkanScreenUiClip::BindScenePassViewport(commandBuffer, ctx.scene, ctx.extent);

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

    for (std::size_t di = 0; di < ctx.scene->transparentDraws.GetSize(); ++di) {
        const SceneDrawItem& d = ctx.scene->transparentDraws[di];
        if (d.skyMode != SceneSkyMode::None) {
            continue;
        }

        const SceneMeshDrawRange range = ResolveSceneMeshDrawRange(d, di, transparentBindings);
        if (!range.drawable) {
            continue;
        }

        VulkanSceneApplyRasterState(commandBuffer, d, d.skyMode);

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
        push.alphaCutoff = 0.0F;
        FillMaterialMapUvPush(push, d);
        push.emissive[0] = d.emissiveColor.x;
        push.emissive[1] = d.emissiveColor.y;
        push.emissive[2] = d.emissiveColor.z;
        push.emissive[3] = d.emissiveIntensity;
        push.emissiveFactor[0] = d.emissiveFactor.x;
        push.emissiveFactor[1] = d.emissiveFactor.y;
        push.emissiveFactor[2] = d.emissiveFactor.z;
        FillMaterialExtensionPush(push, d.gltfExtensions);
        push.albedoHdrLinear = d.textureIsHdrLinear ? 1 : 0;
        push.normalScale = d.normalScale;
        push.useSkinning = 0;
        push.jointCount = 0;
        push.shadingModel = static_cast<std::int32_t>(d.shadingModel);
        push.toonDiffuseBands = std::clamp(d.toonDiffuseBands, 2, 8);
        push.toonRimIntensity = d.toonRimIntensity;
        push.toonRimPower = d.toonRimPower;
        push.normalMapLayer = d.normalMapLayer;
        push.metallicRoughnessMapLayer = d.metallicRoughnessMapLayer;
        push.emissiveMapLayer = d.emissiveMapLayer;
        push.iridescenceThicknessMapLayer = d.iridescenceThicknessMapLayer;

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

void VulkanSceneOpaquePass::CreateRigidInstanceGpuBuffers(
        const VkPhysicalDevice physicalDevice,
        const VkDevice device,
        const std::uint32_t framesInFlight) {
    DestroyRigidInstanceGpuBuffers(device);
    rigidInstanceBuffers.Resize(framesInFlight);
    rigidInstanceMemory.Resize(framesInFlight);
    rigidInstanceMapped.Resize(framesInFlight);
    for (std::uint32_t i = 0; i < framesInFlight; ++i) {
        VulkanRendererGpu::CreateBuffer(
                physicalDevice,
                device,
                static_cast<VkDeviceSize>(kRigidMeshInstanceSsboBytes),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                rigidInstanceBuffers[i],
                rigidInstanceMemory[i]);
        if (vkMapMemory(
                    device,
                    rigidInstanceMemory[i],
                    0,
                    static_cast<VkDeviceSize>(kRigidMeshInstanceSsboBytes),
                    0,
                    &rigidInstanceMapped[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkMapMemory rigid mesh instance SSBO failed");
        }
    }
}

void VulkanSceneOpaquePass::DestroyRigidInstanceGpuBuffers(const VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    for (std::size_t i = 0; i < rigidInstanceBuffers.GetSize(); ++i) {
        if (rigidInstanceMapped[i] != nullptr) {
            vkUnmapMemory(device, rigidInstanceMemory[i]);
            rigidInstanceMapped[i] = nullptr;
        }
        if (rigidInstanceBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, rigidInstanceBuffers[i], nullptr);
            rigidInstanceBuffers[i] = VK_NULL_HANDLE;
        }
        if (rigidInstanceMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, rigidInstanceMemory[i], nullptr);
            rigidInstanceMemory[i] = VK_NULL_HANDLE;
        }
    }
    rigidInstanceBuffers.Clear();
    rigidInstanceMemory.Clear();
    rigidInstanceMapped.Clear();
}

VkBuffer VulkanSceneOpaquePass::RigidInstanceBuffer(const std::uint32_t frameIndex) const noexcept {
    if (frameIndex >= rigidInstanceBuffers.GetSize()) {
        return VK_NULL_HANDLE;
    }
    return rigidInstanceBuffers[frameIndex];
}

bool VulkanSceneOpaquePass::UploadRigidInstances(
        const std::uint32_t frameIndex,
        const SceneRenderParams& scene) const {
    if (frameIndex >= rigidInstanceMapped.GetSize() || rigidInstanceMapped[frameIndex] == nullptr) {
        return false;
    }
    const std::size_t count = scene.meshInstances.GetSize();
    if (count == 0) {
        return true;
    }
    if (count > static_cast<std::size_t>(kMaxRigidMeshInstancesGpu)) {
        return false;
    }
    auto* dst = static_cast<VulkanRigidMeshInstanceGpu*>(rigidInstanceMapped[frameIndex]);
    for (std::size_t i = 0; i < count; ++i) {
        std::memcpy(dst[i].model, scene.meshInstances[i].GetModelMatrix().m, sizeof(dst[i].model));
    }
    return true;
}

void VulkanSceneOpaquePass::RecordRigidInstancedBatches(
        const VkCommandBuffer commandBuffer,
        const VulkanSceneOpaqueRecordContext& ctx) const {
    if (!ctx.sceneParamsValid || ctx.scene == nullptr || ctx.pipelineLitInstanced == VK_NULL_HANDLE ||
        ctx.pipelineLayout == VK_NULL_HANDLE || ctx.descriptorSet == VK_NULL_HANDLE ||
        ctx.customMeshPool == nullptr || ctx.scene->meshInstancedBatches.IsEmpty() ||
        ctx.scene->meshInstances.IsEmpty()) {
        return;
    }
    if (!UploadRigidInstances(ctx.frameIndex, *ctx.scene)) {
        return;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineLitInstanced);
    vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            ctx.pipelineLayout,
            0,
            1,
            &ctx.descriptorSet,
            0,
            nullptr);

    const VkDeviceSize vbOffset = 0;
    SceneMeshGeometryBinding bound = SceneMeshGeometryBinding::None;

    for (std::size_t bi = 0; bi < ctx.scene->meshInstancedBatches.GetSize(); ++bi) {
        const SceneMeshInstancedBatch& batch = ctx.scene->meshInstancedBatches[bi];
        if (batch.GetInstanceCount() == 0U) {
            continue;
        }
        const SceneDrawItem& d = batch.GetDrawTemplate();
        if (d.skyMode != SceneSkyMode::None || d.opacity < 0.999F || d.alphaBlend ||
            d.gltfExtensions.transmissionFactor > 0.01F) {
            continue;
        }

        SceneMeshDrawRange range{};
        if (d.mesh == SceneMeshSlot::UnitCube || d.mesh == SceneMeshSlot::GroundPlane) {
            range = ResolveSceneMeshDrawRange(d, 0, ctx.meshBindings);
        } else if (d.mesh == SceneMeshSlot::Custom && d.customMesh) {
            const CustomMeshGpuSlice slice = ctx.customMeshPool->TryGetRigidMeshSlice(d.customMesh.Get());
            if (slice.indexCount > 0U && ctx.meshBindings.customVertexBuffer != VK_NULL_HANDLE &&
                ctx.meshBindings.customIndexBuffer != VK_NULL_HANDLE) {
                range.drawable = true;
                range.binding = SceneMeshGeometryBinding::CustomDynamic;
                range.indexCount = slice.indexCount;
                range.firstIndex = slice.firstIndex;
                range.vertexOffset = slice.vertexOffset;
            }
        }
        if (!range.drawable || range.indexCount == 0U) {
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

        VulkanSceneApplyRasterState(commandBuffer, d, d.skyMode);

        ModelPushConstants push{};
        std::memcpy(push.model, Matrix4::Identity.m, sizeof(push.model));
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
        push.alphaCutoff = d.alphaCutoff;
        FillMaterialMapUvPush(push, d);
        push.emissive[0] = d.emissiveColor.x;
        push.emissive[1] = d.emissiveColor.y;
        push.emissive[2] = d.emissiveColor.z;
        push.emissive[3] = d.emissiveIntensity;
        push.emissiveFactor[0] = d.emissiveFactor.x;
        push.emissiveFactor[1] = d.emissiveFactor.y;
        push.emissiveFactor[2] = d.emissiveFactor.z;
        FillMaterialExtensionPush(push, d.gltfExtensions);
        push.albedoHdrLinear = d.textureIsHdrLinear ? 1 : 0;
        push.normalScale = d.normalScale;
        push.useSkinning = 0;
        push.jointCount = 0;
        push.shadingModel = static_cast<std::int32_t>(d.shadingModel);
        push.toonDiffuseBands = std::clamp(d.toonDiffuseBands, 2, 8);
        push.toonRimIntensity = d.toonRimIntensity;
        push.toonRimPower = d.toonRimPower;
        push.normalMapLayer = d.normalMapLayer;
        push.metallicRoughnessMapLayer = d.metallicRoughnessMapLayer;
        push.emissiveMapLayer = d.emissiveMapLayer;
        push.iridescenceThicknessMapLayer = d.iridescenceThicknessMapLayer;
        push.instanceBase = batch.GetInstanceBegin();

        vkCmdPushConstants(
                commandBuffer,
                ctx.pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(ModelPushConstants),
                &push);
        vkCmdDrawIndexed(
                commandBuffer,
                range.indexCount,
                batch.GetInstanceCount(),
                range.firstIndex,
                range.vertexOffset,
                0);
    }
}

}  // namespace Spark
