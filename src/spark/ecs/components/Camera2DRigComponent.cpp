#include "spark/ecs/components/Camera2DRigComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/Camera2DComponent.hpp"
#include "spark/ecs/components/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/math/Constants.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 ResolveTargetPosition(const GameObject& target, const Vector3& offset) noexcept {
    const Matrix4& world = target.GetWorldMatrix();
    return {world.m[12] + offset.x, world.m[13] + offset.y, world.m[14] + offset.z};
}

[[nodiscard]] Vector2 ResolveLookAhead(const GameObject& target, const float lookAheadScale) noexcept {
    if (lookAheadScale <= 0.0F) {
        return Vector2::Zero;
    }
    const Rigidbody2DComponent* rb = target.GetComponent<Rigidbody2DComponent>();
    if (rb == nullptr) {
        return Vector2::Zero;
    }
    const Vector2 vel = rb->GetVelocity();
    return vel * lookAheadScale;
}

void ClampCenterToBounds(
        Vector3& center,
        const Vector2& boundsMin,
        const Vector2& boundsMax,
        const float halfExtentY,
        const float aspect) noexcept {
    const float halfH = halfExtentY;
    const float halfW = halfH * std::max(aspect, Epsilon);
    const float minX = boundsMin.x + halfW;
    const float maxX = boundsMax.x - halfW;
    const float minY = boundsMin.y + halfH;
    const float maxY = boundsMax.y - halfH;
    if (minX <= maxX) {
        center.x = std::clamp(center.x, minX, maxX);
    } else {
        center.x = (boundsMin.x + boundsMax.x) * 0.5F;
    }
    if (minY <= maxY) {
        center.y = std::clamp(center.y, minY, maxY);
    } else {
        center.y = (boundsMin.y + boundsMax.y) * 0.5F;
    }
}

}  // namespace

void Camera2DRigComponent::Tick(
        Camera2DRigComponent& rig,
        GameObject& owner,
        const float deltaSeconds,
        const float framebufferAspect) noexcept {
    if (rig.mode == Camera2DRigMode::Manual) {
        return;
    }
    TransformComponent* camTr = owner.GetComponent<TransformComponent>();
    Camera2DComponent* cam = owner.GetComponent<Camera2DComponent>();
    if (camTr == nullptr || cam == nullptr || !cam->IsEnabled()) {
        return;
    }

    if (rig.useZoomLimits) {
        float halfY = cam->GetHalfExtentY();
        halfY = std::clamp(halfY, rig.zoomMinHalfExtentY, rig.zoomMaxHalfExtentY);
        cam->SetHalfExtentY(halfY);
    }

    GameObject* followTarget = rig.target;
    if (followTarget == nullptr) {
        return;
    }

    Vector3 desired = ResolveTargetPosition(*followTarget, rig.targetOffset);
    const Vector2 ahead = ResolveLookAhead(*followTarget, rig.lookAheadScale);
    desired.x += ahead.x;
    desired.y += ahead.y;

    Vector3 current = camTr->GetLocalTransform().translation;
    const float t = std::min(1.0F, std::max(0.0F, rig.followSmoothRate * deltaSeconds));
    current.x += (desired.x - current.x) * t;
    current.y += (desired.y - current.y) * t;
    current.z = desired.z;

    if (rig.mode == Camera2DRigMode::BoundedFollow || rig.useBounds) {
        ClampCenterToBounds(
                current,
                rig.boundsMin,
                rig.boundsMax,
                cam->GetHalfExtentY(),
                framebufferAspect);
    }

    camTr->SetTranslation(current);
}

void Camera2DRigComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    float aspect = 1.0F;
    if (fbH > 0) {
        aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
    }
    Tick(*this, owner, timing.deltaTimeSeconds, aspect);
}

}  // namespace Spark
