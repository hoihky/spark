#include "spark/scene/camera/WorldSpaceScreenProject.hpp"

#include "spark/math/Constants.hpp"
#include "spark/math/Vector4.hpp"

#include <cmath>

namespace Spark {

bool TryWorldToScreenPixels(
        const Matrix4& viewProjection,
        const Vector3& worldPosition,
        const float framebufferWidth,
        const float framebufferHeight,
        float& outScreenX,
        float& outScreenY) noexcept {
    if (framebufferWidth < Epsilon || framebufferHeight < Epsilon) {
        return false;
    }
    const Vector4 clip = viewProjection * Vector4(worldPosition.x, worldPosition.y, worldPosition.z, 1.0F);
    if (std::fabs(clip.w) < Epsilon) {
        return false;
    }
    const float invW = 1.0F / clip.w;
    const float ndcX = clip.x * invW;
    const float ndcY = clip.y * invW;
    const float ndcZ = clip.z * invW;
    if (ndcX < -1.0F || ndcX > 1.0F || ndcY < -1.0F || ndcY > 1.0F || ndcZ < 0.0F || ndcZ > 1.0F) {
        return false;
    }
    outScreenX = (ndcX + 1.0F) * 0.5F * framebufferWidth;
    outScreenY = (ndcY + 1.0F) * 0.5F * framebufferHeight;
    return true;
}

}  // namespace Spark