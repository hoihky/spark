#include "spark/physics/CharacterController3D.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/ColliderBakePipeline3D.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

struct ControllerBody {
    GameObject* obj = nullptr;
    CharacterController3DComponent* controller = nullptr;
    TransformComponent* transform = nullptr;
};

[[nodiscard]] Vector3 HomogeneousPoint(const Matrix4& worldMatrix, const Vector3& local) noexcept {
    const Vector4 p = worldMatrix * Vector4(local.x, local.y, local.z, 1.0F);
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

void ComputeCharacterControllerWorld(
        GameObject& owner,
        const CharacterController3DComponent& controller,
        Vector3& outCenter,
        float& outRadius) noexcept {
    const Matrix4 worldMatrix = owner.GetWorldMatrix();
    outCenter = HomogeneousPoint(worldMatrix, controller.GetCenterOffset());
    const float sx = std::sqrt(
            worldMatrix.m[0] * worldMatrix.m[0] + worldMatrix.m[1] * worldMatrix.m[1] +
            worldMatrix.m[2] * worldMatrix.m[2]);
    const float sy = std::sqrt(
            worldMatrix.m[4] * worldMatrix.m[4] + worldMatrix.m[5] * worldMatrix.m[5] +
            worldMatrix.m[6] * worldMatrix.m[6]);
    const float sz = std::sqrt(
            worldMatrix.m[8] * worldMatrix.m[8] + worldMatrix.m[9] * worldMatrix.m[9] +
            worldMatrix.m[10] * worldMatrix.m[10]);
    const float scale = std::max({sx, sy, sz});
    outRadius = controller.GetRadius() * scale;
}

void ApplyTranslationDelta(TransformComponent& transform, const Vector3& centerDelta) noexcept {
    Vector3 translation = transform.GetLocalTransform().translation;
    translation.x += centerDelta.x;
    translation.y += centerDelta.y;
    translation.z += centerDelta.z;
    transform.SetTranslation(translation);
}

[[nodiscard]] bool AnyStaticOverlap(
        const Vector3& center,
        const float radius,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    CollisionAabb3 query{};
    query.minX = center.x - radius;
    query.minY = center.y - radius;
    query.minZ = center.z - radius;
    query.maxX = center.x + radius;
    query.maxY = center.y + radius;
    query.maxZ = center.z + radius;
    broadPhase.QueryUniquePayloadIndices(query, scratch);
    for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
        const std::uint32_t idx = scratch[i];
        if (idx >= colliders.GetSize()) {
            continue;
        }
        if (Collider3DOverlapsSphere(colliders[idx], center, radius)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] float ComputeTranslationLambdaAgainstStatics(
        const Vector3& startCenter,
        const Vector3& endCenter,
        const float radius,
        const int binaryIterations,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    if (binaryIterations <= 0) {
        return 1.0F;
    }
    if (!AnyStaticOverlap(endCenter, radius, colliders, broadPhase, scratch)) {
        return 1.0F;
    }
    if (!AnyStaticOverlap(startCenter, radius, colliders, broadPhase, scratch)) {
        float lo = 0.0F;
        float hi = 1.0F;
        for (int k = 0; k < binaryIterations; ++k) {
            const float mid = 0.5F * (lo + hi);
            const Vector3 midCenter{
                    startCenter.x + (endCenter.x - startCenter.x) * mid,
                    startCenter.y + (endCenter.y - startCenter.y) * mid,
                    startCenter.z + (endCenter.z - startCenter.z) * mid};
            if (AnyStaticOverlap(midCenter, radius, colliders, broadPhase, scratch)) {
                hi = mid;
            } else {
                lo = mid;
            }
        }
        return std::max(0.0F, lo * 0.999F);
    }
    return 0.0F;
}

void DepenetrateSphere(
        Vector3& center,
        const float radius,
        const float skinWidth,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    constexpr int kMaxPasses = 6;
    for (int pass = 0; pass < kMaxPasses; ++pass) {
        CollisionAabb3 query{};
        query.minX = center.x - radius - skinWidth;
        query.minY = center.y - radius - skinWidth;
        query.minZ = center.z - radius - skinWidth;
        query.maxX = center.x + radius + skinWidth;
        query.maxY = center.y + radius + skinWidth;
        query.maxZ = center.z + radius + skinWidth;
        broadPhase.QueryUniquePayloadIndices(query, scratch);

        float bestPen = 0.0F;
        float nx = 0.0F;
        float ny = 0.0F;
        float nz = 0.0F;
        bool found = false;
        for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
            const std::uint32_t idx = scratch[i];
            if (idx >= colliders.GetSize()) {
                continue;
            }
            float pen = 0.0F;
            float cx = 0.0F;
            float cy = 0.0F;
            float cz = 0.0F;
            if (!ComputeSphereStaticCollider3Contact(center, radius, colliders[idx], cx, cy, cz, pen, skinWidth)) {
                continue;
            }
            if (!found || pen > bestPen) {
                found = true;
                bestPen = pen;
                nx = cx;
                ny = cy;
                nz = cz;
            }
        }
        if (!found || bestPen <= 1.0e-8F) {
            break;
        }
        center.x += nx * bestPen;
        center.y += ny * bestPen;
        center.z += nz * bestPen;
    }
}

void MoveSphereWithSlide(
        ControllerBody& body,
        Vector3 displacement,
        const float radius,
        const float skinWidth,
        const int slideIterations,
        const int sweepBinaryIterations,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    Vector3 center{};
    float worldRadius = 0.0F;
    ComputeCharacterControllerWorld(*body.obj, *body.controller, center, worldRadius);
    (void)worldRadius;
    const float useRadius = radius;

    for (int slide = 0; slide < slideIterations; ++slide) {
        const float remainLen = displacement.Length();
        if (remainLen < 1.0e-6F) {
            break;
        }

        const Vector3 startCenter = center;
        const Vector3 targetCenter{
                center.x + displacement.x, center.y + displacement.y, center.z + displacement.z};
        const float lambda = ComputeTranslationLambdaAgainstStatics(
                startCenter, targetCenter, useRadius, sweepBinaryIterations, colliders, broadPhase, scratch);
        const Vector3 moved{
                startCenter.x + displacement.x * lambda,
                startCenter.y + displacement.y * lambda,
                startCenter.z + displacement.z * lambda};
        const Vector3 delta{moved.x - center.x, moved.y - center.y, moved.z - center.z};
        center = moved;
        ApplyTranslationDelta(*body.transform, delta);

        if (lambda >= 0.999F) {
            displacement = Vector3::Zero;
            break;
        }

        CollisionAabb3 query{};
        query.minX = center.x - useRadius;
        query.minY = center.y - useRadius;
        query.minZ = center.z - useRadius;
        query.maxX = center.x + useRadius;
        query.maxY = center.y + useRadius;
        query.maxZ = center.z + useRadius;
        broadPhase.QueryUniquePayloadIndices(query, scratch);

        float nx = 0.0F;
        float ny = 0.0F;
        float nz = 0.0F;
        float pen = 0.0F;
        bool hit = false;
        for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
            const std::uint32_t idx = scratch[i];
            if (idx >= colliders.GetSize()) {
                continue;
            }
            float cx = 0.0F;
            float cy = 0.0F;
            float cz = 0.0F;
            float cp = 0.0F;
            if (!ComputeSphereStaticCollider3Contact(center, useRadius, colliders[idx], cx, cy, cz, cp, skinWidth)) {
                continue;
            }
            if (!hit || cp > pen) {
                hit = true;
                pen = cp;
                nx = cx;
                ny = cy;
                nz = cz;
            }
        }
        if (!hit) {
            break;
        }

        const Vector3 remaining{
                displacement.x * (1.0F - lambda), displacement.y * (1.0F - lambda), displacement.z * (1.0F - lambda)};
        const float into = remaining.x * nx + remaining.y * ny + remaining.z * nz;
        displacement.x = remaining.x - nx * into;
        displacement.y = remaining.y - ny * into;
        displacement.z = remaining.z - nz * into;
    }

    DepenetrateSphere(center, useRadius, skinWidth, colliders, broadPhase, scratch);
    const Vector3 finalCenter = center;
    Vector3 currentCenter{};
    float ignored = 0.0F;
    ComputeCharacterControllerWorld(*body.obj, *body.controller, currentCenter, ignored);
    ApplyTranslationDelta(
            *body.transform,
            {finalCenter.x - currentCenter.x, finalCenter.y - currentCenter.y, finalCenter.z - currentCenter.z});
}

[[nodiscard]] bool ProbeGrounded(
        const Vector3& center,
        const float radius,
        const float skinWidth,
        const float slopeLimitCos,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    const Vector3 probe{center.x, center.y - skinWidth - radius * 0.08F, center.z};
    CollisionAabb3 query{};
    query.minX = probe.x - radius;
    query.minY = probe.y - radius;
    query.minZ = probe.z - radius;
    query.maxX = probe.x + radius;
    query.maxY = probe.y + radius;
    query.maxZ = probe.z + radius;
    broadPhase.QueryUniquePayloadIndices(query, scratch);

    for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
        const std::uint32_t idx = scratch[i];
        if (idx >= colliders.GetSize()) {
            continue;
        }
        float nx = 0.0F;
        float ny = 0.0F;
        float nz = 0.0F;
        float pen = 0.0F;
        if (!ComputeSphereStaticCollider3Contact(probe, radius, colliders[idx], nx, ny, nz, pen, skinWidth)) {
            continue;
        }
        if (ny >= slopeLimitCos) {
            return true;
        }
    }
    return false;
}

void TryStepOffset(
        ControllerBody& body,
        const Vector3& horizontalDelta,
        const float radius,
        const float stepOffset,
        const float skinWidth,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    if (stepOffset <= 1.0e-5F) {
        return;
    }
    Vector3 center{};
    float worldRadius = 0.0F;
    ComputeCharacterControllerWorld(*body.obj, *body.controller, center, worldRadius);
    (void)worldRadius;

    const Vector3 upStep{0.0F, stepOffset, 0.0F};
    Vector3 stepped = center;
    stepped.y += stepOffset;
    if (AnyStaticOverlap(stepped, radius, colliders, broadPhase, scratch)) {
        return;
    }
    ApplyTranslationDelta(*body.transform, upStep);

    Vector3 afterForward = stepped;
    afterForward.x += horizontalDelta.x;
    afterForward.z += horizontalDelta.z;
    if (AnyStaticOverlap(afterForward, radius, colliders, broadPhase, scratch)) {
        ApplyTranslationDelta(*body.transform, {0.0F, -stepOffset, 0.0F});
        return;
    }
    ApplyTranslationDelta(*body.transform, {horizontalDelta.x, 0.0F, horizontalDelta.z});

    Vector3 downTarget = afterForward;
    downTarget.y -= stepOffset;
    const float lambda = ComputeTranslationLambdaAgainstStatics(
            afterForward, downTarget, radius, 8, colliders, broadPhase, scratch);
    const float drop = stepOffset * lambda;
    ApplyTranslationDelta(*body.transform, {0.0F, -drop, 0.0F});
    (void)skinWidth;
}

void CollectControllers(GameWorld& world, Array<ControllerBody>& out) noexcept {
    out.Clear();
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        auto* controller = object->GetComponent<CharacterController3DComponent>();
        auto* transform = object->GetComponent<TransformComponent>();
        if (controller == nullptr || transform == nullptr) {
            return;
        }
        ControllerBody body{};
        body.obj = object;
        body.controller = controller;
        body.transform = transform;
        out.PushBack(body);
    });
}

}  // namespace

void CharacterControllerWorld3D::Simulate(GameWorld& world, const FrameTiming& timing) {
    const float dt = timing.deltaTimeSeconds;
    if (dt <= 0.0F) {
        return;
    }

    Array<Collider3D> colliders;
    SpatialHashGrid3D broadPhase;
    ColliderBakePipeline3D::GetDefault().Rebuild(world, settings.broadPhaseCellSize, colliders, broadPhase);

    Array<std::uint32_t> scratch;
    Array<ControllerBody> controllers;
    CollectControllers(world, controllers);

    for (std::size_t ci = 0; ci < controllers.GetSize(); ++ci) {
        ControllerBody& body = controllers[ci];
        CharacterController3DComponent& cc = *body.controller;

        Vector3 velocity = cc.velocity;
        velocity.x = cc.moveInput.x;
        velocity.z = cc.moveInput.z;

        if (!cc.grounded) {
            velocity.y += settings.gravityY * cc.gravityScale * dt;
        }
        velocity.y = std::clamp(velocity.y, -settings.maxFallSpeed, settings.maxFallSpeed);

        Vector3 center{};
        float radius = 0.0F;
        ComputeCharacterControllerWorld(*body.obj, cc, center, radius);

        const float slopeLimitCos = std::cos(cc.slopeLimitDegrees * (Pi / 180.0F));
        cc.grounded = ProbeGrounded(center, radius, cc.skinWidth, slopeLimitCos, colliders, broadPhase, scratch);
        if (cc.grounded && velocity.y < 0.0F) {
            velocity.y = 0.0F;
        }

        const Vector3 horizontalDelta{velocity.x * dt, 0.0F, velocity.z * dt};
        const Vector3 verticalDelta{0.0F, velocity.y * dt, 0.0F};

        if (horizontalDelta.LengthSquared() > 1.0e-10F) {
            const Vector3 before = body.transform->GetLocalTransform().translation;
            MoveSphereWithSlide(
                    body,
                    horizontalDelta,
                    radius,
                    cc.skinWidth,
                    settings.slideIterations,
                    settings.sweepBinaryIterations,
                    colliders,
                    broadPhase,
                    scratch);
            const Vector3 after = body.transform->GetLocalTransform().translation;
            const Vector3 applied{after.x - before.x, after.y - before.y, after.z - before.z};
            if ((applied.x * applied.x + applied.z * applied.z) <
                    (horizontalDelta.x * horizontalDelta.x + horizontalDelta.z * horizontalDelta.z) * 0.25F) {
                TryStepOffset(body, horizontalDelta, radius, cc.stepOffset, cc.skinWidth, colliders, broadPhase, scratch);
            }
        }

        ComputeCharacterControllerWorld(*body.obj, cc, center, radius);
        MoveSphereWithSlide(
                body,
                verticalDelta,
                radius,
                cc.skinWidth,
                settings.slideIterations,
                settings.sweepBinaryIterations,
                colliders,
                broadPhase,
                scratch);

        ComputeCharacterControllerWorld(*body.obj, cc, center, radius);
        cc.grounded = ProbeGrounded(center, radius, cc.skinWidth, slopeLimitCos, colliders, broadPhase, scratch);
        if (cc.grounded && velocity.y < 0.0F) {
            velocity.y = 0.0F;
        }

        cc.velocity = velocity;
        cc.moveInput = Vector3::Zero;
    }
}

void SimulateCharacterControllers3D(
        GameWorld& world,
        const FrameTiming& timing,
        const CharacterController3DSettings& settings) {
    CharacterControllerWorld3D controllerWorld(settings);
    controllerWorld.Simulate(world, timing);
}

}  // namespace Spark
