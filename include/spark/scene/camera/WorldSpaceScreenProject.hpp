#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * Projects a world-space point to framebuffer pixels (origin top-left, Y down) using the same
 * <c>viewProjection</c> as <c>SceneRenderParams</c> / <c>Camera2D::ViewProjection</c>.
 * Returns false when clip <c>w</c> is near zero or the point is outside the clip volume (NDC).
 */
[[nodiscard]] bool TryWorldToScreenPixels(
        const Matrix4& viewProjection,
        const Vector3& worldPosition,
        float framebufferWidth,
        float framebufferHeight,
        float& outScreenX,
        float& outScreenY) noexcept;

}  // namespace Spark