#include "spark/physics/simulation/RigidbodyIntegrator3D.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/DynamicBody3D.hpp"
#include "spark/physics/simulation/SweptCcd3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

void ApplyTranslationDeltaToBody(DynamicBody3D& body, const Vector3& delta) noexcept {
    Vector3 translation = body.tr->GetLocalTransform().translation;
    translation.x += delta.x;
    translation.y += delta.y;
    translation.z += delta.z;
    body.tr->SetTranslation(translation);
    body.collider.Translate(delta);
}

void ApplyLinearDamping(Vector3& v, const float dampingPerSec, const float dt) noexcept {
    if (dampingPerSec <= 0.0F) {
        return;
    }
    const float k = std::exp(-dampingPerSec * dt);
    v.x *= k;
    v.y *= k;
    v.z *= k;
}

void ApplyAngularDamping(Vector3& w, const float dampingPerSec, const float dt) noexcept {
    if (dampingPerSec <= 0.0F) {
        return;
    }
    const float k = std::exp(-dampingPerSec * dt);
    w.x *= k;
    w.y *= k;
    w.z *= k;
}

void IntegrateLocalOrientation(TransformComponent& tr, const Vector3& w, const float dt) noexcept {
    const float w2 = w.x * w.x + w.y * w.y + w.z * w.z;
    if (w2 < 1.0e-20F) {
        return;
    }
    Quaternion q = tr.GetLocalTransform().rotation.Normalized();
    const Quaternion wq{w.x, w.y, w.z, 0.0F};
    const Quaternion qDot = wq * q;
    const float halfDt = 0.5F * dt;
    const Quaternion dq{qDot.x * halfDt, qDot.y * halfDt, qDot.z * halfDt, qDot.w * halfDt};
    q = Quaternion{q.x + dq.x, q.y + dq.y, q.z + dq.z, q.w + dq.w}.Normalized();
    tr.SetRotation(q);
}

}  // namespace

void RigidbodyIntegrator3D::IntegrateSubstep(
        Array<DynamicBody3D>& bodies,
        const float substepDt,
        const PhysicsWorld3DSettings& settings,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& queryScratch) noexcept {
    for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
        DynamicBody3D& body = bodies[bi];
        Vector3 v = body.rb->GetVelocity();
        v.y += settings.gravityY * body.rb->GetGravityScale() * substepDt;
        v.y = std::clamp(v.y, -settings.maxFallSpeed, settings.maxFallSpeed);
        ApplyLinearDamping(v, body.rb->GetLinearDamping(), substepDt);
        body.rb->SetVelocity(v);

        Vector3 w = body.rb->GetAngularVelocity();
        ApplyAngularDamping(w, body.rb->GetAngularDamping(), substepDt);
        body.rb->SetAngularVelocity(w);

        RefreshDynamicBody3D(body);
        const Vector3 track0 = body.collider.GetTrackingPoint();
        const Vector3 track1{track0.x + v.x * substepDt, track0.y + v.y * substepDt, track0.z + v.z * substepDt};
        const float lambda = SweptCcd3D::ComputeTranslationLambdaAgainstStatics(
                body.collider,
                track0,
                track1,
                settings.sweptStaticCcdBinaryIterations,
                colliders,
                broadPhase,
                queryScratch);
        const Vector3 trackSafe{
                track0.x + (track1.x - track0.x) * lambda,
                track0.y + (track1.y - track0.y) * lambda,
                track0.z + (track1.z - track0.z) * lambda};
        ApplyTranslationDeltaToBody(body, {trackSafe.x - track0.x, trackSafe.y - track0.y, trackSafe.z - track0.z});

        IntegrateLocalOrientation(*body.tr, w, substepDt);
    }
}

}  // namespace Spark
