#pragma once

#include <cstdint>

namespace Spark {

class RenderTexture;

/**
 * Read-only view of a GPU-resident offscreen color (+ optional depth) target.
 * Created via <c>IRenderTargetService::CreateRenderTarget</c>.
 */
class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    [[nodiscard]] virtual const RenderTexture& GetColorTexture() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t GetWidth() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t GetHeight() const noexcept = 0;
    [[nodiscard]] virtual bool HasDepth() const noexcept = 0;
};

}  // namespace Spark
