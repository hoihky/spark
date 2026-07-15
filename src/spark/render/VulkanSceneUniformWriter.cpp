#include "spark/render/VulkanSceneUniformWriter.hpp"

#include "spark/render/VulkanClusteredLightGpu.hpp"
#include "spark/render/VulkanDirectionalShadowCascadeMath.hpp"
#include "spark/render/VulkanSceneUniformGpu.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>
#include <cstddef>
#include <cstring>

namespace Spark {

void VulkanSceneUniformWriter::Write(
        void* mappedUbo,
        const SceneRenderParams& scene,
        const ResolvedSceneLighting& lighting,
        const VkExtent2D swapchainExtent,
        const VulkanDirectionalShadowPass& shadowPass,
        const std::uint32_t frameIndex,
        VulkanDirectionalShadowFrameState& shadowOut) const {
    using Ubo = SceneUniformGpu;
    static_assert(offsetof(Ubo, ambientSky) == 128);
    static_assert(offsetof(Ubo, ambientProbe) == 144);
    static_assert(offsetof(Ubo, invViewProj) == 160);
    static_assert(offsetof(Ubo, worldToShadowClip) == 256);
    static_assert(offsetof(Ubo, cascadeSplits) == 512);
    static_assert(offsetof(Ubo, cascadeAtlas) == 528);
    static_assert(offsetof(Ubo, shadowParams) == 592);
    static_assert(offsetof(Ubo, clusterGrid) == 608);
    static_assert(offsetof(Ubo, iblParams) == 640);
    static_assert(sizeof(Ubo) == 656);
    if (mappedUbo == nullptr) {
        return;
    }
    constexpr VkDeviceSize kUboAllocSize = kSceneUniformGpuBytes;
    std::memset(mappedUbo, 0, static_cast<std::size_t>(kUboAllocSize));

    const Vector3 lightDir = scene.lightDirectionWorld.Normalized();
    SceneUniformGpu u{};
    std::memcpy(u.viewProj, scene.viewProjection.m, sizeof(u.viewProj));
    Matrix4 invVp{};
    if (scene.viewProjection.TryInvert(invVp)) {
        std::memcpy(u.invViewProj, invVp.m, sizeof(u.invViewProj));
    } else {
        std::memcpy(u.invViewProj, Matrix4::Identity.m, sizeof(u.invViewProj));
    }
    u.viewportSize[0] = static_cast<float>((swapchainExtent.width > 0) ? swapchainExtent.width : 1);
    u.viewportSize[1] = static_cast<float>((swapchainExtent.height > 0) ? swapchainExtent.height : 1);
    u.viewportSize[2] = lighting.shadowDistanceFadeEnd;
    u.viewportSize[3] = scene.shadowDepthSampleFlipV ? 1.0F : 0.0F;
    u.timeGlobal[0] = scene.sceneTimeSeconds;
    u.timeGlobal[1] = lighting.shadowDistanceFadeStartRatio;
    u.timeGlobal[2] = scene.useTimeOfDay ? scene.timeOfDay : 0.0F;
    u.timeGlobal[3] = lighting.shadowCascadeBlendFraction;
    u.lightDir[0] = lightDir.x;
    u.lightDir[1] = lightDir.y;
    u.lightDir[2] = lightDir.z;
    u.lightDir[3] = 0.0F;
    u.cameraPos[0] = scene.cameraPositionWorld.x;
    u.cameraPos[1] = scene.cameraPositionWorld.y;
    u.cameraPos[2] = scene.cameraPositionWorld.z;
    u.cameraPos[3] = 0.0F;
    u.lightColor[0] = scene.lightColor.x;
    u.lightColor[1] = scene.lightColor.y;
    u.lightColor[2] = scene.lightColor.z;
    u.lightColor[3] = scene.lightIntensity;
    const SceneAmbientSettings& amb = lighting.ambient;
    const float ambScale = lighting.ambientScale;
    u.ambientColor[0] = amb.groundColor.x * ambScale;
    u.ambientColor[1] = amb.groundColor.y * ambScale;
    u.ambientColor[2] = amb.groundColor.z * ambScale;
    u.ambientColor[3] = 1.0F;
    u.ambientSky[0] = amb.skyColor.x * ambScale;
    u.ambientSky[1] = amb.skyColor.y * ambScale;
    u.ambientSky[2] = amb.skyColor.z * ambScale;
    u.ambientSky[3] = 1.0F;
    u.ambientProbe[0] = amb.probeColor.x * ambScale;
    u.ambientProbe[1] = amb.probeColor.y * ambScale;
    u.ambientProbe[2] = amb.probeColor.z * ambScale;
    u.ambientProbe[3] = amb.probeWeight;

    shadowOut.active = false;
    for (std::uint32_t ci = 0; ci < VulkanDirectionalShadowPass::kCascadeCount; ++ci) {
        std::memcpy(u.worldToShadowClip[ci], Matrix4::Identity.m, sizeof(u.worldToShadowClip[ci]));
        const std::uint32_t col = ci % 2U;
        const std::uint32_t row = ci / 2U;
        u.cascadeAtlas[ci][0] = static_cast<float>(col) * 0.5F;
        u.cascadeAtlas[ci][1] = static_cast<float>(row) * 0.5F;
        u.cascadeAtlas[ci][2] = 0.5F;
        u.cascadeAtlas[ci][3] = 0.5F;
    }
    u.shadowParams[0] = scene.shadowBias;
    u.shadowParams[1] = scene.shadowNormalBias;
    u.shadowParams[2] = 1.0F / static_cast<float>(VulkanDirectionalShadowPass::kCascadeTileSize);
    u.shadowParams[3] = 0.0F;
    if (lighting.directionalShadowsEnabled && scene.directionalShadowsEnabled && shadowPass.HasFlightDepthView(frameIndex) &&
        shadowPass.CompareSampler() != VK_NULL_HANDLE) {
        constexpr float kLambda = 0.5F;
        const float kNear = lighting.shadowCascadeNear;
        const float kFar = lighting.shadowCascadeFar;
        float distSplits[VulkanDirectionalShadowPass::kCascadeCount + 1]{};
        for (std::uint32_t si = 0; si <= VulkanDirectionalShadowPass::kCascadeCount; ++si) {
            distSplits[si] = VulkanDirectionalShadowCascadeMath::ComputeSplitDistance(
                    kNear,
                    kFar,
                    static_cast<int>(si),
                    static_cast<int>(VulkanDirectionalShadowPass::kCascadeCount),
                    kLambda);
        }
        bool anyCascade = false;
        for (std::uint32_t ci = 0; ci < VulkanDirectionalShadowPass::kCascadeCount; ++ci) {
            u.cascadeSplits[ci] = distSplits[ci + 1];
            shadowOut.cascadeSplits[ci] = distSplits[ci + 1];
            Matrix4 w2s{};
            if (VulkanDirectionalShadowCascadeMath::ComputeWorldToShadowClip(
                        scene.viewProjection,
                        lightDir,
                        distSplits[ci],
                        distSplits[ci + 1],
                        VulkanDirectionalShadowPass::kCascadeTileSize,
                        w2s)) {
                std::memcpy(u.worldToShadowClip[ci], w2s.m, sizeof(u.worldToShadowClip[ci]));
                shadowOut.worldToShadowClip[ci] = w2s;
                anyCascade = true;
            }
        }
        if (anyCascade) {
            u.shadowParams[3] = 1.0F;
            shadowOut.active = true;
            for (std::uint32_t ci = 0; ci < VulkanDirectionalShadowPass::kCascadeCount; ++ci) {
                std::memcpy(shadowOut.cascadeAtlas[ci], u.cascadeAtlas[ci], sizeof(shadowOut.cascadeAtlas[ci]));
            }
        }
    }

    const std::uint32_t npl = static_cast<std::uint32_t>(std::min(
            scene.pointLights.GetSize(),
            static_cast<std::size_t>(SceneRenderParams::MaxPointLights)));
    const std::uint32_t nsl = static_cast<std::uint32_t>(std::min(
            scene.spotLights.GetSize(),
            static_cast<std::size_t>(SceneRenderParams::MaxSpotLights)));
    u.clusterGrid[0] = static_cast<float>(kClusterGridX);
    u.clusterGrid[1] = static_cast<float>(kClusterGridY);
    u.clusterGrid[2] = static_cast<float>(kClusterGridZ);
    u.clusterGrid[3] = static_cast<float>(npl);
    const float clusterNear = std::max(lighting.shadowCascadeNear, 0.05F);
    const float clusterFar = std::max(lighting.shadowCascadeFar, clusterNear + 1.0F);
    u.clusterDepth[0] = clusterNear;
    u.clusterDepth[1] = clusterFar;
    u.clusterDepth[2] = static_cast<float>(nsl);
    u.clusterDepth[3] = (npl > 0 || nsl > 0) ? 1.0F : 0.0F;

    u.iblParams[0] = static_cast<float>(scene.iblEnvironmentLayer);
    u.iblParams[1] = scene.iblIntensity;
    u.iblParams[2] = 0.0F;
    u.iblParams[3] = scene.iblEnabled ? 1.0F : 0.0F;

    std::memcpy(mappedUbo, &u, sizeof(SceneUniformGpu));
}

}  // namespace Spark
