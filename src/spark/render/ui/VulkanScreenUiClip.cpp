#include "spark/render/ui/VulkanScreenUiClip.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

bool VulkanScreenUiClip::SameClip(
        const bool aEn,
        const float ax,
        const float ay,
        const float aw,
        const float ah,
        const bool bEn,
        const float bx,
        const float by,
        const float bw,
        const float bh) noexcept {
    if (aEn != bEn) {
        return false;
    }
    if (!aEn) {
        return true;
    }
    return ax == bx && ay == by && aw == bw && ah == bh;
}

bool VulkanScreenUiClip::SameClipRectDraw(const ScreenRectDraw& a, const ScreenRectDraw& b) noexcept {
    return SameClip(
            a.clipEnabled,
            a.clipX,
            a.clipY,
            a.clipW,
            a.clipH,
            b.clipEnabled,
            b.clipX,
            b.clipY,
            b.clipW,
            b.clipH);
}

bool VulkanScreenUiClip::SameSolidRectBatch(const ScreenRectDraw& a, const ScreenRectDraw& b) noexcept {
    return a.blendMode == b.blendMode && SameClipRectDraw(a, b);
}

bool VulkanScreenUiClip::SameClipTextDraw(const ScreenTextDraw& a, const ScreenTextDraw& b) noexcept {
    return SameClip(
            a.clipEnabled,
            a.clipX,
            a.clipY,
            a.clipW,
            a.clipH,
            b.clipEnabled,
            b.clipX,
            b.clipY,
            b.clipW,
            b.clipH);
}

bool VulkanScreenUiClip::SameTextBatch(const ScreenTextDraw& a, const ScreenTextDraw& b) noexcept {
    return a.blendMode == b.blendMode && SameClipTextDraw(a, b);
}

bool VulkanScreenUiClip::ScreenRectToVkScissor(
        const bool clipEnabled,
        const float clipX,
        const float clipY,
        const float clipW,
        const float clipH,
        const VkExtent2D fb,
        VkRect2D& out) noexcept {
    if (!clipEnabled) {
        out.offset = {0, 0};
        out.extent = fb;
        return true;
    }
    const int32_t x0 = static_cast<int32_t>(std::floor(clipX));
    const int32_t y0 = static_cast<int32_t>(std::floor(clipY));
    const int32_t x1 = static_cast<int32_t>(std::ceil(clipX + clipW));
    const int32_t y1 = static_cast<int32_t>(std::ceil(clipY + clipH));
    const int32_t fbw = static_cast<int32_t>(fb.width);
    const int32_t fbh = static_cast<int32_t>(fb.height);
    const int32_t cx0 = std::max<int32_t>(0, x0);
    const int32_t cy0 = std::max<int32_t>(0, y0);
    const int32_t cx1 = std::min(x1, fbw);
    const int32_t cy1 = std::min(y1, fbh);
    const int32_t w = cx1 - cx0;
    const int32_t h = cy1 - cy0;
    if (w <= 0 || h <= 0) {
        return false;
    }
    out.offset = {cx0, cy0};
    out.extent = {static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h)};
    return true;
}

void VulkanScreenUiClip::BindScenePassScissor(
        const VkCommandBuffer commandBuffer,
        const SceneRenderParams* scene,
        const VkExtent2D extent,
        VkRect2D& outFullFramebuffer) noexcept {
    outFullFramebuffer.offset = {0, 0};
    outFullFramebuffer.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &outFullFramebuffer);
    if (scene != nullptr && scene->worldViewportScissorEnabled) {
        VkRect2D worldScissor{};
        if (ScreenRectToVkScissor(
                    true,
                    scene->worldViewportScissorX,
                    scene->worldViewportScissorY,
                    scene->worldViewportScissorW,
                    scene->worldViewportScissorH,
                    extent,
                    worldScissor)) {
            vkCmdSetScissor(commandBuffer, 0, 1, &worldScissor);
        }
    }
}

void VulkanScreenUiClip::RestoreFramebufferScissor(
        const VkCommandBuffer commandBuffer, const VkRect2D& fullFramebuffer) noexcept {
    vkCmdSetScissor(commandBuffer, 0, 1, &fullFramebuffer);
}

}  // namespace Spark
