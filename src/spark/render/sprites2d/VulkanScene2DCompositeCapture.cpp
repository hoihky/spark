#include "spark/render/sprites2d/VulkanScene2DCompositeCapture.hpp"

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/render/scene/VulkanOffscreenRenderTarget.hpp"
#include "spark/scene/camera/Camera2D.hpp"
#include "spark/scene/render/RenderTexture.hpp"
#include "spark/scene/render/RenderTextureFormat.hpp"

#include <algorithm>

namespace Spark {

void VulkanScene2DCompositeCapture::Record(VkCommandBuffer commandBuffer, const RecordContext& ctx) const noexcept {
    if (commandBuffer == VK_NULL_HANDLE || !ctx.sceneParamsValid || ctx.scene == nullptr || ctx.renderTargets == nullptr ||
        ctx.descriptors == nullptr || ctx.uniformWriter == nullptr || ctx.lighting == nullptr ||
        ctx.directionalShadow == nullptr || ctx.shadowFrameState == nullptr || ctx.tilemapPass == nullptr ||
        ctx.spritePass == nullptr || ctx.compositePass == nullptr || ctx.hdrRenderPass == VK_NULL_HANDLE) {
        return;
    }

    const SceneRenderParams& scene = *ctx.scene;
    if (scene.scene2DCompositeViews.IsEmpty()) {
        return;
    }

    void* const uniformMapped = ctx.descriptors->UniformMapped(ctx.frameIndex);
    if (uniformMapped == nullptr) {
        return;
    }

    const Matrix4 savedViewProj = scene.viewProjection;
    const Vector3 savedCameraPos = scene.cameraPositionWorld;

    for (std::size_t vi = 0; vi < scene.scene2DCompositeViews.GetSize(); ++vi) {
        const Scene2DCompositeViewDesc& view = scene.scene2DCompositeViews[vi];
        if (!view.enabled || !view.target) {
            continue;
        }
        RenderTexture& rt = *view.target;
        ctx.renderTargets->EnsureGpuResources(rt);
        VulkanOffscreenRenderTarget* gpu = ctx.renderTargets->TryGetGpu(rt);
        if (gpu == nullptr || !gpu->IsAllocated() || gpu->Framebuffer() == VK_NULL_HANDLE) {
            continue;
        }

        const VkExtent2D rtExtent = gpu->Extent();
        if (rtExtent.width == 0U || rtExtent.height == 0U) {
            continue;
        }

        Camera2D orthoCam{};
        orthoCam.position = view.worldCenter;
        orthoCam.halfExtentY = std::max(view.worldOrthoHalfExtent, 0.25F);
        const Matrix4 orthoVp =
                orthoCam.ViewProjection(static_cast<float>(rtExtent.width), static_cast<float>(rtExtent.height));

        SceneRenderParams captureScene = scene;
        captureScene.viewProjection = orthoVp;
        captureScene.cameraPositionWorld = view.worldCenter;
        captureScene.worldClearColorEnabled = true;
        captureScene.worldClearColor = {0.78F, 0.70F, 0.55F};
        captureScene.exposure = 1.35F;
        captureScene.lightIntensity = std::max(captureScene.lightIntensity, 0.85F);
        captureScene.ambientScale = std::max(captureScene.ambientScale, 1.25F);
        ctx.uniformWriter->Write(
                uniformMapped,
                captureScene,
                *ctx.lighting,
                rtExtent,
                *ctx.directionalShadow,
                ctx.frameIndex,
                *ctx.shadowFrameState);

        gpu->TransitionColorImage(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        if (gpu->DepthView() != VK_NULL_HANDLE) {
            gpu->TransitionDepthImage(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        VkClearValue clears[2]{};
        clears[0].color = {{0.78F, 0.70F, 0.55F, 1.0F}};
        clears[1].depthStencil = {1.0F, 0};

        const bool ldrCapture = rt.GetColorFormat() == RenderTextureFormat::Rgba8Unorm;
        const VkRenderPass captureRenderPass =
                ctx.renderTargets != nullptr ? ctx.renderTargets->RenderPassFor(rt) : ctx.hdrRenderPass;
        if (captureRenderPass == VK_NULL_HANDLE) {
            continue;
        }

        VkRenderPassBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = captureRenderPass;
        beginInfo.framebuffer = gpu->Framebuffer();
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderArea.extent = rtExtent;
        beginInfo.clearValueCount = gpu->DepthView() != VK_NULL_HANDLE ? 2U : 1U;
        beginInfo.pClearValues = clears;
        vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        const VulkanTilemapRecordContext tilemapCtx{
                .scene = ctx.scene,
                .sceneParamsValid = ctx.sceneParamsValid,
                .frameIndex = ctx.frameIndex,
                .extent = rtExtent,
                .vertexBuffer = ctx.vertexBuffer,
                .indexBuffer = ctx.indexBuffer,
                .quadFirstIndex = ctx.quadFirstIndex,
                .quadIndexCount = ctx.quadIndexCount,
                .descriptorSet = ctx.descriptors->DescriptorSet(ctx.frameIndex),
                .ldrOffscreenTarget = ldrCapture,
        };
        const VulkanSpriteRecordContext spriteCtx{
                .scene = ctx.scene,
                .sceneParamsValid = ctx.sceneParamsValid,
                .frameIndex = ctx.frameIndex,
                .extent = rtExtent,
                .vertexBuffer = ctx.vertexBuffer,
                .indexBuffer = ctx.indexBuffer,
                .quadFirstIndex = ctx.quadFirstIndex,
                .quadIndexCount = ctx.quadIndexCount,
                .descriptorSet = ctx.descriptors->DescriptorSet(ctx.frameIndex),
                .ldrOffscreenTarget = ldrCapture,
        };
        ctx.compositePass->Record(commandBuffer, *ctx.tilemapPass, *ctx.spritePass, tilemapCtx, spriteCtx);

        vkCmdEndRenderPass(commandBuffer);
        gpu->SyncColorLayoutAfterHdrRenderPassEnd();
        gpu->TransitionColorImage(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    }

    SceneRenderParams restoreScene = scene;
    restoreScene.viewProjection = savedViewProj;
    restoreScene.cameraPositionWorld = savedCameraPos;
    VkExtent2D presentExtent = ctx.presentExtent;
    if (presentExtent.width == 0U) {
        presentExtent.width = 1U;
    }
    if (presentExtent.height == 0U) {
        presentExtent.height = 1U;
    }
    ctx.uniformWriter->Write(
            uniformMapped,
            restoreScene,
            *ctx.lighting,
            presentExtent,
            *ctx.directionalShadow,
            ctx.frameIndex,
            *ctx.shadowFrameState);
}

}  // namespace Spark
