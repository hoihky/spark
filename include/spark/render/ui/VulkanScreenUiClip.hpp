#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/** Screen-space clip rect helpers for batched UI draw recording. */
class VulkanScreenUiClip {
public:
    [[nodiscard]] static bool SameClip(
            bool aEn,
            float ax,
            float ay,
            float aw,
            float ah,
            bool bEn,
            float bx,
            float by,
            float bw,
            float bh) noexcept;

    [[nodiscard]] static bool SameClipRectDraw(const ScreenRectDraw& a, const ScreenRectDraw& b) noexcept;

    /** Clip and compositing mode must match to batch solid UI geometry. */
    [[nodiscard]] static bool SameSolidRectBatch(const ScreenRectDraw& a, const ScreenRectDraw& b) noexcept;

    [[nodiscard]] static bool SameClipTextDraw(const ScreenTextDraw& a, const ScreenTextDraw& b) noexcept;

    /** Clip and compositing mode must match to batch text geometry. */
    [[nodiscard]] static bool SameTextBatch(const ScreenTextDraw& a, const ScreenTextDraw& b) noexcept;

    /** Returns false when the clip rect is enabled but degenerates after clamping. */
    [[nodiscard]] static bool ScreenRectToVkScissor(
            bool clipEnabled,
            float clipX,
            float clipY,
            float clipW,
            float clipH,
            VkExtent2D framebufferExtent,
            VkRect2D& out) noexcept;

    /** Full-frame scissor; narrows to <c>worldViewportScissor*</c> when enabled. <c>outFullFramebuffer</c> is for restore. */
    static void BindScenePassScissor(
            VkCommandBuffer commandBuffer,
            const SceneRenderParams* scene,
            VkExtent2D extent,
            VkRect2D& outFullFramebuffer) noexcept;

    static void RestoreFramebufferScissor(VkCommandBuffer commandBuffer, const VkRect2D& fullFramebuffer) noexcept;
};

}  // namespace Spark
