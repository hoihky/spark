#include "spark/render/shadow/VulkanShadowCastDraw.hpp"

#include "spark/math/Matrix4.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/render/scene/VulkanSceneMeshDraw.hpp"

#include <cstring>

namespace Spark {

void RecordShadowCastMeshes(
        const VkCommandBuffer commandBuffer,
        const VkPipeline pipeline,
        const VkPipelineLayout pipelineLayout,
        const VulkanShadowRecordContext& ctx,
        const float lightViewProj[16],
        const std::uint32_t frameIndex) {
    if (!ctx.sceneParamsValid || ctx.scene == nullptr || ctx.vertexBuffer == VK_NULL_HANDLE ||
        ctx.indexBuffer == VK_NULL_HANDLE || pipeline == VK_NULL_HANDLE || pipelineLayout == VK_NULL_HANDLE) {
        return;
    }

    enum class ShadowGeom : std::uint8_t { None, StaticScene, CustomDynamic };

    const SceneMeshDrawBindings meshBindings{
            .staticVertexBuffer = ctx.vertexBuffer,
            .staticIndexBuffer = ctx.indexBuffer,
            .customVertexBuffer = ctx.customVertexBuffer,
            .customIndexBuffer = ctx.customIndexBuffer,
            .cubeIndexCount = ctx.cubeIndexCount,
            .planeIndexCount = ctx.planeIndexCount,
            .planeFirstIndex = ctx.planeFirstIndex,
            .cubeVertexOffset = ctx.cubeVertexOffset,
            .planeVertexOffset = ctx.planeVertexOffset,
            .customDrawPacked = ctx.customDrawPacked,
    };

    VulkanShadowPushConstants sh{};
    std::memcpy(sh.lightViewProj, lightViewProj, sizeof(sh.lightViewProj));

    ShadowGeom bound = ShadowGeom::None;

    for (std::size_t di = 0; di < ctx.scene->draws.GetSize(); ++di) {
        const SceneDrawItem& d = ctx.scene->draws[di];
        if (d.skyMode != SceneSkyMode::None) {
            continue;
        }
        if ((d.shadowFlags & kSceneShadowCast) == 0) {
            continue;
        }

        const SceneMeshDrawRange range = ResolveSceneMeshDrawRange(d, di, meshBindings);
        if (!range.drawable) {
            continue;
        }

        if (range.binding == SceneMeshGeometryBinding::StaticScene) {
            if (bound != ShadowGeom::StaticScene) {
                const VkDeviceSize vbOff = 0;
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ctx.vertexBuffer, &vbOff);
                vkCmdBindIndexBuffer(commandBuffer, ctx.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                bound = ShadowGeom::StaticScene;
            }
        } else if (range.binding == SceneMeshGeometryBinding::CustomDynamic) {
            if (bound != ShadowGeom::CustomDynamic) {
                const VkDeviceSize vbOff = 0;
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ctx.customVertexBuffer, &vbOff);
                vkCmdBindIndexBuffer(commandBuffer, ctx.customIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
                bound = ShadowGeom::CustomDynamic;
            }
        }

        sh.useSkinning = 0;
        sh.jointCount = 0;
        sh.sp0 = 0;
        sh.sp1 = 0;
        if (!d.jointPalette.IsEmpty() && d.skinnedMesh && ctx.skinSsboMapped != nullptr &&
            frameIndex < ctx.skinSsboMapped->GetSize() && (*ctx.skinSsboMapped)[frameIndex] != nullptr) {
            const std::uint32_t jc = static_cast<std::uint32_t>(d.jointPalette.GetSize());
            if (jc > 0 && jc <= ctx.maxSkinJoints) {
                sh.useSkinning = 1;
                sh.jointCount = static_cast<std::int32_t>(jc);
                std::memcpy(
                        (*ctx.skinSsboMapped)[frameIndex],
                        d.jointPalette.GetData(),
                        static_cast<std::size_t>(jc) * sizeof(Matrix4));
            }
        }

        std::memcpy(sh.model, d.model.m, sizeof(sh.model));
        vkCmdPushConstants(
                commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(VulkanShadowPushConstants),
                &sh);
        vkCmdDrawIndexed(commandBuffer, range.indexCount, 1, range.firstIndex, range.vertexOffset, 0);
    }
}

}  // namespace Spark
