#include "spark/ecs/components/camera/SpringArm3DComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector4.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Hp3(const Vector4& p) noexcept {
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

[[nodiscard]] Vector3 PivotWorld(const GameObject& pivot, const Vector3& socketOffset) noexcept {
    const Matrix4& world = pivot.GetWorldMatrix();
    const Vector4 localOffset{socketOffset.x, socketOffset.y, socketOffset.z, 1.0F};
    const Vector3 p = Hp3(world * localOffset);
    return p;
}

}  // namespace

void SpringArm3DComponent::Tick(SpringArm3DComponent& arm, GameObject& owner) noexcept {
    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }
    GameObject* pivot = arm.pivotTarget;
    if (pivot == nullptr) {
        pivot = owner.GetParent();
    }
    if (pivot == nullptr) {
        return;
    }

    const Vector3 pivotPos = PivotWorld(*pivot, arm.socketOffset);
    const float cp = std::cos(arm.pitchRadians);
    const float sp = std::sin(arm.pitchRadians);
    const float cy = std::cos(arm.yawRadians);
    const float sy = std::sin(arm.yawRadians);
    Vector3 back{
            cp * sy,
            sp,
            cp * cy};
    const float bl2 = back.LengthSquared();
    if (bl2 > Epsilon) {
        back = back * (1.0F / std::sqrt(bl2));
    }

    float length = std::max(arm.minArmLength, arm.armLength);
    // Simple probe: shorten arm when camera would intersect ground plane y=0 (demo-friendly).
    const float groundY = 0.02F;
    if (back.y < -1.0e-4F) {
        const float tHit = (groundY + arm.probeRadius - pivotPos.y) / back.y;
        if (tHit > 0.0F && tHit < length) {
            length = std::max(arm.minArmLength, tHit);
        }
    }

    const Vector3 cameraPos = pivotPos + back * length;
    tr->SetTranslation(cameraPos);
    const Vector3 toPivot = pivotPos - cameraPos;
    const float tl2 = toPivot.LengthSquared();
    if (tl2 > Epsilon) {
        const Vector3 forward = toPivot * (1.0F / std::sqrt(tl2));
        tr->SetRotation(Quaternion::FromShortestArc(Vector3{0.0F, 0.0F, -1.0F}, forward));
    }
}

void SpringArm3DComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& /*context*/) {
    Tick(*this, owner);
}

}  // namespace Spark
