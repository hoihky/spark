#pragma once

#include "spark/memory/SharedPtr.hpp"

namespace Spark {

class IRenderTarget;
class RenderTexture;

/**
 * Allocates and releases GPU images for <c>RenderTexture</c> assets.
 * Implemented by the active frame presenter (Vulkan). Default engine hooks return nullptr.
 */
class IRenderTargetService {
public:
    virtual ~IRenderTargetService() = default;

    /** Ensures GPU color/depth images exist and match <c>texture</c>'s current desc. */
    virtual void EnsureGpuResources(RenderTexture& texture) = 0;

    /**
     * Returns a view over @p texture after ensuring GPU allocation.
     * The view keeps GPU resources alive until the last <c>IRenderTarget</c> is released.
     */
    [[nodiscard]] virtual SharedPtr<IRenderTarget> CreateRenderTarget(SharedPtr<RenderTexture> texture) = 0;

    /** Drops GPU resources for @p texture (safe if no active views). */
    virtual void ReleaseRenderTarget(const RenderTexture& texture) noexcept = 0;
};

}  // namespace Spark
