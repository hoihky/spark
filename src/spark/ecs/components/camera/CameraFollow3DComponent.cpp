#include "spark/ecs/components/camera/CameraFollow3DComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Quaternion.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 TargetWorldPosition(const GameObject& target, const Vector3& offset) noexcept {
    const Matrix4& world = target.GetWorldMatrix();
    return {world.m[12] + offset.x, world.m[13] + offset.y, world.m[14] + offset.z};
}

}  // namespace

void CameraFollow3DComponent::Tick(
        CameraFollow3DComponent& rig,
        GameObject& owner,
        const float deltaSeconds) noexcept {
    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (tr == nullptr || rig.target == nullptr) {
        return;
    }
    const Vector3 desired = TargetWorldPosition(*rig.target, rig.targetOffset);
    Vector3 current = tr->GetLocalTransform().translation;
    const float t = std::min(1.0F, std::max(0.0F, rig.followSmoothRate * deltaSeconds));
    current.x += (desired.x - current.x) * t;
    current.y += (desired.y - current.y) * t;
    current.z += (desired.z - current.z) * t;
    tr->SetTranslation(current);

    if (!rig.lookAtTarget) {
        return;
    }
    Vector3 toTarget = TargetWorldPosition(*rig.target, Vector3::Zero) - current;
    const float len2 = toTarget.LengthSquared();
    if (len2 <= 1.0e-8F) {
        return;
    }
    toTarget = toTarget * (1.0F / std::sqrt(len2));
    const Quaternion look = Quaternion::FromShortestArc(Vector3{0.0F, 0.0F, -1.0F}, toTarget);
    tr->SetRotation(look);
}

void CameraFollow3DComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    Tick(*this, owner, timing.deltaTimeSeconds);
}

}  // namespace Spark
