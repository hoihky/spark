#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/render/scene/VulkanSceneMeshDraw.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>

namespace Spark {

struct VulkanSceneOpaqueRecordContext {
    const SceneRenderParams* scene = nullptr;
    bool sceneParamsValid = false;
    std::uint32_t frameIndex = 0;
    VkExtent2D extent{};
    VkPipeline pipelineLit = VK_NULL_HANDLE;
    VkPipeline pipelineSky = VK_NULL_HANDLE;
    VkPipeline pipelineLitTransparent = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    SceneMeshDrawBindings meshBindings{};
    Array<void*>* skinSsboMapped = nullptr;
    std::uint32_t maxSkinJoints = 64;
};

/** Lit + sky mesh draws into the HDR color pass (shares scene descriptor set). */
class VulkanSceneOpaquePass {
public:
    struct ModelPushConstants {
        float model[16]{};
        float albedo[4]{1.0F, 1.0F, 1.0F, 1.0F};
        std::int32_t textureLayer = -1;
        std::int32_t skyMode = 0;
        float metallic = 0.0F;
        float roughness = 0.45F;
        float emissive[4]{};
        std::int32_t useSkinning = 0;
        std::int32_t jointCount = 0;
        std::int32_t shadingModel = 0;
        std::int32_t toonDiffuseBands = 3;
        float toonRimIntensity = 0.35F;
        float toonRimPower = 4.0F;
        std::int32_t normalMapLayer = -1;
        std::int32_t metallicRoughnessMapLayer = -1;
        std::int32_t emissiveMapLayer = -1;
        std::int32_t iridescenceThicknessMapLayer = -1;
        float metallicFactor = 1.0F;
        float roughnessFactor = 1.0F;
        float occlusionStrength = 1.0F;
        std::int32_t shadowFlags = 0;
        float alphaCutoff = 0.0F;
        /** Matches GLSL vec2[] 8-byte alignment (SPIR-V offset 176). */
        std::uint8_t padMapUvAlign[4]{};
        float mapUvScale[5][2];
        float mapUvOffset[5][2];
        float mapUvRotation[5]{};
        std::int32_t mapTexCoordSet[5]{};
        /** KHR_materials_clearcoat factors. */
        float clearcoatFactor = 0.0F;
        float clearcoatRoughnessFactor = 0.0F;
        float emissiveStrength = 1.0F;
        float transmissionFactor = 0.0F;
        /** glTF emissiveFactor RGB. */
        float emissiveFactor[3]{1.0F, 1.0F, 1.0F};
        std::int32_t albedoHdrLinear = 0;
        float normalScale = 1.0F;
        float iridescenceFactor = 0.0F;
        float iridescenceIor = 1.3F;
        float iridescenceThicknessMin = 100.0F;
        float iridescenceThicknessMax = 400.0F;
    };

    void Record(VkCommandBuffer commandBuffer, const VulkanSceneOpaqueRecordContext& ctx) const;
    /** Non-sky lit meshes (used by <c>VulkanWaterPass</c> until a dedicated water shader exists). */
    void RecordLitMeshDraws(
            VkCommandBuffer commandBuffer,
            const VulkanSceneOpaqueRecordContext& ctx,
            const Array<SceneDrawItem>& draws,
            const Array<CustomMeshGpuSlice>& customPacked) const;
    void RecordTransparent(
            VkCommandBuffer commandBuffer,
            const VulkanSceneOpaqueRecordContext& ctx,
            const Array<CustomMeshGpuSlice>& transparentPacked) const;
};

}  // namespace Spark
