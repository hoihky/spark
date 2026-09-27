#include "spark/ecs/components/camera/CameraBounds2DComponent.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

void CameraBounds2DComponent::SetHalfExtents(const Vector2& value) noexcept {
    halfExtents.x = std::max(0.01F, value.x);
    halfExtents.y = std::max(0.01F, value.y);
}

void CameraBounds2DComponent::ComputeWorldBounds(const GameObject& owner, CollisionAabb2& out) const noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    const Vector3 worldPos = world.TranslationVector();
    float scaleX = 1.0F;
    float scaleY = 1.0F;
    if (const TransformComponent* tr = owner.GetComponent<TransformComponent>()) {
        const Vector3 scale = tr->GetLocalTransform().scale;
        scaleX = std::max(0.01F, std::fabs(scale.x));
        scaleY = std::max(0.01F, std::fabs(scale.y));
    }
    const float centerX = worldPos.x + localOffset.x;
    const float centerY = worldPos.y + localOffset.y;
    const float halfW = halfExtents.x * scaleX;
    const float halfH = halfExtents.y * scaleY;
    out.minX = centerX - halfW;
    out.maxX = centerX + halfW;
    out.minY = centerY - halfH;
    out.maxY = centerY + halfH;
}

bool CameraBounds2DComponent::ContainsWorldPoint(
        const GameObject& owner,
        const float worldX,
        const float worldY) const noexcept {
    if (!enabled) {
        return false;
    }
    CollisionAabb2 bounds{};
    ComputeWorldBounds(owner, bounds);
    return worldX >= bounds.minX && worldX <= bounds.maxX && worldY >= bounds.minY && worldY <= bounds.maxY;
}

}  // namespace Spark
