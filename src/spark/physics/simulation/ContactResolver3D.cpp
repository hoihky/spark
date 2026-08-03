#include "spark/physics/simulation/ContactResolver3D.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/DynamicBody3D.hpp"
#include "spark/physics/colliders/DynamicCollider3D.hpp"
#include "spark/physics/core/ColliderMaterial.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Spark {

namespace ContactResolver3DDetail {

void ApplyTranslationDeltaToBody(DynamicBody3D& body, const Vector3& delta) noexcept {
    Vector3 translation = body.tr->GetLocalTransform().translation;
    translation.x += delta.x;
    translation.y += delta.y;
    translation.z += delta.z;
    body.tr->SetTranslation(translation);
    body.collider.Translate(delta);
}

[[nodiscard]] float Clamp01(const float x) noexcept {
    return std::clamp(x, 0.0F, 1.0F);
}

[[nodiscard]] float CombineGeometricMean(const float a, const float b) noexcept {
    return std::sqrt(std::max(1.0e-8F, a) * std::max(1.0e-8F, b));
}

[[nodiscard]] float SurfaceRestitutionOrNeutral(const GameObject* o) noexcept {
    if (o == nullptr) {
        return 1.0F;
    }
    const PhysicsMaterial3DComponent* m = o->GetComponent<PhysicsMaterial3DComponent>();
    if (m == nullptr) {
        return 1.0F;
    }
    return Clamp01(m->GetRestitution());
}

[[nodiscard]] float SurfaceDynamicFrictionOrDefault(const GameObject* o) noexcept {
    if (o == nullptr) {
        return 0.55F;
    }
    const PhysicsMaterial3DComponent* m = o->GetComponent<PhysicsMaterial3DComponent>();
    if (m == nullptr) {
        return 0.55F;
    }
    return std::max(0.0F, m->GetDynamicFriction());
}

[[nodiscard]] float PairRestitution(
        const Rigidbody3DComponent& dynRb,
        const GameObject* dynObj,
        const Collider3D& col) noexcept {
    const ColliderMaterial& mat = col.GetMaterial();
    if (!mat.isDefined) {
        return Clamp01(dynRb.GetRestitution());
    }
    const float geom = CombineGeometricMean(SurfaceRestitutionOrNeutral(dynObj), Clamp01(mat.restitution));
    return Clamp01(dynRb.GetRestitution() * geom);
}

[[nodiscard]] float PairDynamicFriction(const GameObject* dynObj, const Collider3D& col) noexcept {
    const ColliderMaterial& mat = col.GetMaterial();
    if (!mat.isDefined) {
        return 0.0F;
    }
    return CombineGeometricMean(SurfaceDynamicFrictionOrDefault(dynObj), std::max(0.0F, mat.dynamicFriction));
}

[[nodiscard]] float PairRestitutionDynamicDynamic(
        const Rigidbody3DComponent& a,
        const GameObject* oa,
        const Rigidbody3DComponent& b,
        const GameObject* ob) noexcept {
    const float sa = SurfaceRestitutionOrNeutral(oa);
    const float sb = SurfaceRestitutionOrNeutral(ob);
    const float surface = CombineGeometricMean(sa, sb);
    const float body = CombineGeometricMean(Clamp01(a.GetRestitution()), Clamp01(b.GetRestitution()));
    return Clamp01(surface * body);
}

[[nodiscard]] float PairFrictionDynamicDynamic(const GameObject* oa, const GameObject* ob) noexcept {
    return CombineGeometricMean(SurfaceDynamicFrictionOrDefault(oa), SurfaceDynamicFrictionOrDefault(ob));
}

void ApplyVelocityRestitutionAlongNormal(
        Vector3& v, const float nx, const float ny, const float nz, const float restitution) noexcept {
    const float e = Clamp01(restitution);
    const float vn = v.x * nx + v.y * ny + v.z * nz;
    if (vn >= 0.0F) {
        return;
    }
    const float impulse = (1.0F + e) * vn;
    v.x -= impulse * nx;
    v.y -= impulse * ny;
    v.z -= impulse * nz;
}

void ApplyCoulombKineticFrictionTangent(
        Vector3& v,
        const float nx,
        const float ny,
        const float nz,
        const float muDyn,
        const float vnApproach,
        const float restitution,
        const float minImpactNormalSpeed) noexcept {
    if (muDyn <= 0.0F) {
        return;
    }
    const float e = Clamp01(restitution);
    const float vn = v.x * nx + v.y * ny + v.z * nz;
    float tx = v.x - nx * vn;
    float ty = v.y - ny * vn;
    float tz = v.z - nz * vn;
    const float t2 = tx * tx + ty * ty + tz * tz;
    if (t2 < 1.0e-16F) {
        return;
    }
    if (vnApproach >= -std::max(0.0F, minImpactNormalSpeed)) {
        return;
    }
    const float tLen = std::sqrt(t2);
    const float slipCap = muDyn * (1.0F + e) * std::fabs(vnApproach);
    const float newLen = std::max(0.0F, tLen - slipCap);
    const float s = newLen / tLen;
    v.x = nx * vn + tx * s;
    v.y = ny * vn + ty * s;
    v.z = nz * vn + tz * s;
}

void ApplyDeepestStaticNormalImpulse(
        DynamicBody3D& body,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch,
        const float h,
        const PhysicsWorld3DSettings& settings) noexcept {
    RefreshDynamicBody3D(body);

    constexpr float kBroadInflate = 0.006F;
    CollisionAabb3 q = body.collider.GetBounds();
    q.minX -= kBroadInflate;
    q.minY -= kBroadInflate;
    q.minZ -= kBroadInflate;
    q.maxX += kBroadInflate;
    q.maxY += kBroadInflate;
    q.maxZ += kBroadInflate;
    broadPhase.QueryUniquePayloadIndices(q, scratch);

    Vector3 vel = body.rb->GetVelocity();
    Vector3 omega = body.rb->GetAngularVelocity();
    const Vector3 anchor = body.collider.GetTrackingPoint();

    constexpr float kNarrowSlop = 0.0045F;
    constexpr float kApproachEps = 1.0e-6F;
    constexpr int kMaxPasses = 6;

    for (int pass = 0; pass < kMaxPasses; ++pass) {
        std::uint32_t bestIdx = UINT32_MAX;
        float bestVn = 0.0F;
        float bestPen = 0.0F;
        float bnX = 0.0F;
        float bnY = 0.0F;
        float bnZ = 0.0F;
        bool have = false;

        for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
            const std::uint32_t idx = scratch[i];
            if (idx >= colliders.GetSize()) {
                continue;
            }
            const Collider3D& rec = colliders[idx];
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            if (!body.collider.ComputeStaticContact(rec, nx, ny, nz, pen, kNarrowSlop)) {
                continue;
            }
            const float vn = vel.x * nx + vel.y * ny + vel.z * nz;
            if (vn >= -kApproachEps) {
                continue;
            }
            if (!have || vn < bestVn || (vn <= bestVn + 1.0e-5F && pen > bestPen)) {
                have = true;
                bestVn = vn;
                bestPen = pen;
                bnX = nx;
                bnY = ny;
                bnZ = nz;
                bestIdx = idx;
            }
        }
        if (!have || bestIdx == UINT32_MAX) {
            break;
        }

        const float vn0 = bestVn;
        const Collider3D& rec = colliders[bestIdx];
        const float ePair = PairRestitution(*body.rb, body.obj, rec);
        const float mu = PairDynamicFriction(body.obj, rec);
        const Vector3 vPhys0 = vel;
        ApplyVelocityRestitutionAlongNormal(vel, bnX, bnY, bnZ, ePair);
        ApplyCoulombKineticFrictionTangent(
                vel, bnX, bnY, bnZ, mu, vn0, ePair, settings.frictionImpactNormalSpeed);

        const float invM = body.rb->GetInverseMass();
        const float invI = body.collider.EffectiveInverseInertia(*body.rb);
        if (invM > 1.0e-10F && invI > 1.0e-10F) {
            const Vector3 dv{vel.x - vPhys0.x, vel.y - vPhys0.y, vel.z - vPhys0.z};
            const Vector3 impulse{dv.x / invM, dv.y / invM, dv.z / invM};
            const float contactRadius = body.collider.GetImpulseContactRadius();
            Vector3 contactOnBody{
                    anchor.x - bnX * contactRadius,
                    anchor.y - bnY * contactRadius,
                    anchor.z - bnZ * contactRadius};
            const Vector3 rArm{
                    contactOnBody.x - anchor.x, contactOnBody.y - anchor.y, contactOnBody.z - anchor.z};
            const Vector3 tau = Vector3::Cross(rArm, impulse);
            omega.x += invI * tau.x;
            omega.y += invI * tau.y;
            omega.z += invI * tau.z;
        }

        if (settings.baumgarteContactBias > 0.0F && bestPen > settings.contactPenetrationSlop) {
            const float hh = std::max(h, 1.0e-6F);
            const float bias = std::min(
                    settings.baumgarteMaxSeparationVelocity, settings.baumgarteContactBias * bestPen / hh);
            vel.x += bias * bnX;
            vel.y += bias * bnY;
            vel.z += bias * bnZ;
        }
    }

    body.rb->SetVelocity(vel);
    body.rb->SetAngularVelocity(omega);
}

void ApplyDynamicPairVelocityImpulses(
        Vector3& va,
        Vector3& vb,
        Vector3* omegaA,
        const float invIA,
        const float wA,
        const DynamicCollider3D& simA,
        Vector3* omegaB,
        const float invIB,
        const float wB,
        const DynamicCollider3D& simB,
        const float nx,
        const float ny,
        const float nz,
        const float restitution,
        const float muDyn,
        const float frictionMinImpactNormalSpeed,
        const float h,
        const PhysicsWorld3DSettings& settings,
        const float pen) noexcept {
    const Vector3 vaPhys0 = va;
    const Vector3 vbPhys0 = vb;
    const float vnRel0 = (va.x - vb.x) * nx + (va.y - vb.y) * ny + (va.z - vb.z) * nz;
    const float e = Clamp01(restitution);
    if (vnRel0 > 1.0e-5F) {
        const float den = wA + wB;
        if (den > 1.0e-10F) {
            const float j = (1.0F + e) * vnRel0 / den;
            va.x -= nx * j * wA;
            va.y -= ny * j * wA;
            va.z -= nz * j * wA;
            vb.x += nx * j * wB;
            vb.y += ny * j * wB;
            vb.z += nz * j * wB;
        }
    }

    if (muDyn > 0.0F && vnRel0 > std::max(1.0e-5F, frictionMinImpactNormalSpeed)) {
        const float slipCapEach = 0.5F * muDyn * (1.0F + e) * vnRel0;
        auto applyPairFriction = [&](Vector3& v, const float nx2, const float ny2, const float nz2, const float cap) {
            const float vnn = v.x * nx2 + v.y * ny2 + v.z * nz2;
            float tx = v.x - nx2 * vnn;
            float ty = v.y - ny2 * vnn;
            float tz = v.z - nz2 * vnn;
            const float t2b = tx * tx + ty * ty + tz * tz;
            if (t2b < 1.0e-16F) {
                return;
            }
            const float tLenB = std::sqrt(t2b);
            const float newLenB = std::max(0.0F, tLenB - cap);
            const float sB = newLenB / tLenB;
            v.x = nx2 * vnn + tx * sB;
            v.y = ny2 * vnn + ty * sB;
            v.z = nz2 * vnn + tz * sB;
        };
        applyPairFriction(va, nx, ny, nz, slipCapEach);
        applyPairFriction(vb, nx, ny, nz, slipCapEach);
    }

    if (omegaA != nullptr && invIA > 0.0F && wA > 1.0e-10F) {
        const Vector3 dv{va.x - vaPhys0.x, va.y - vaPhys0.y, va.z - vaPhys0.z};
        const Vector3 impulse{dv.x / wA, dv.y / wA, dv.z / wA};
        Vector3 contactOnA{};
        ComputeDynamicDynamicContactPointOnA(simA, simB, nx, ny, nz, contactOnA);
        const Vector3 anchorA = simA.GetTrackingPoint();
        const Vector3 rA{contactOnA.x - anchorA.x, contactOnA.y - anchorA.y, contactOnA.z - anchorA.z};
        const Vector3 tau = Vector3::Cross(rA, impulse);
        omegaA->x += invIA * tau.x;
        omegaA->y += invIA * tau.y;
        omegaA->z += invIA * tau.z;
    }
    if (omegaB != nullptr && invIB > 0.0F && wB > 1.0e-10F) {
        const Vector3 dv{vb.x - vbPhys0.x, vb.y - vbPhys0.y, vb.z - vbPhys0.z};
        const Vector3 impulse{dv.x / wB, dv.y / wB, dv.z / wB};
        Vector3 contactOnB{};
        ComputeDynamicDynamicContactPointOnA(simB, simA, -nx, -ny, -nz, contactOnB);
        const Vector3 anchorB = simB.GetTrackingPoint();
        const Vector3 rB{contactOnB.x - anchorB.x, contactOnB.y - anchorB.y, contactOnB.z - anchorB.z};
        const Vector3 tau = Vector3::Cross(rB, impulse);
        omegaB->x += invIB * tau.x;
        omegaB->y += invIB * tau.y;
        omegaB->z += invIB * tau.z;
    }

    if (settings.baumgarteContactBias > 0.0F && pen > settings.contactPenetrationSlop) {
        const float hh = std::max(h, 1.0e-6F);
        const float bias =
                std::min(settings.baumgarteMaxSeparationVelocity, settings.baumgarteContactBias * pen / hh);
        va.x += bias * nx;
        va.y += bias * ny;
        va.z += bias * nz;
        vb.x -= bias * nx;
        vb.y -= bias * ny;
        vb.z -= bias * nz;
    }
}


}  // namespace ContactResolver3DDetail

using namespace ContactResolver3DDetail;

void ContactResolver3D::ApplyStaticNormalImpulses(
        Array<DynamicBody3D>& bodies,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch,
        const float substepDt,
        const PhysicsWorld3DSettings& settings) noexcept {
    for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
        ApplyDeepestStaticNormalImpulse(bodies[bi], colliders, broadPhase, scratch, substepDt, settings);
    }
}

void ContactResolver3D::ResolvePenetrations(
        Array<DynamicBody3D>& bodies,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch,
        const PhysicsWorld3DSettings& settings) noexcept {
    const int iters = std::max(1, settings.resolveIterations);
    for (int it = 0; it < iters; ++it) {
        for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
            DynamicBody3D& body = bodies[bi];
            RefreshDynamicBody3D(body);

            broadPhase.QueryUniquePayloadIndices(body.collider.GetBounds(), scratch);
            for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
                const std::uint32_t idx = scratch[i];
                if (idx >= colliders.GetSize()) {
                    continue;
                }
                const Collider3D& rec = colliders[idx];
                if (!body.collider.OverlapsStatic(rec)) {
                    continue;
                }
                const Vector3 trackBefore = body.collider.GetTrackingPoint();
                DynamicCollider3D separated =
                        DynamicCollider3D::FromLegacySnapshot(body.collider.ToLegacySnapshot());
                if (separated.SeparateFromStatic(rec)) {
                    const Vector3 trackAfter = separated.GetTrackingPoint();
                    ApplyTranslationDeltaToBody(
                            body,
                            {trackAfter.x - trackBefore.x, trackAfter.y - trackBefore.y, trackAfter.z - trackBefore.z});
                }
            }
        }

        for (std::size_t a = 0; a < bodies.GetSize(); ++a) {
            for (std::size_t b = a + 1; b < bodies.GetSize(); ++b) {
                DynamicBody3D& bodyA = bodies[a];
                DynamicBody3D& bodyB = bodies[b];
                RefreshDynamicBody3D(bodyA);
                RefreshDynamicBody3D(bodyB);

                const Vector3 trackA0 = bodyA.collider.GetTrackingPoint();
                const Vector3 trackB0 = bodyB.collider.GetTrackingPoint();
                DynamicCollider3D simA = DynamicCollider3D::FromLegacySnapshot(bodyA.collider.ToLegacySnapshot());
                DynamicCollider3D simB = DynamicCollider3D::FromLegacySnapshot(bodyB.collider.ToLegacySnapshot());
                if (simA.SeparateFromDynamic(simB, bodyA.rb->GetInverseMass(), bodyB.rb->GetInverseMass())) {
                    const Vector3 trackA1 = simA.GetTrackingPoint();
                    const Vector3 trackB1 = simB.GetTrackingPoint();
                    ApplyTranslationDeltaToBody(
                            bodyA,
                            {trackA1.x - trackA0.x, trackA1.y - trackA0.y, trackA1.z - trackA0.z});
                    ApplyTranslationDeltaToBody(
                            bodyB,
                            {trackB1.x - trackB0.x, trackB1.y - trackB0.y, trackB1.z - trackB0.z});
                }
            }
        }
    }
}

void ContactResolver3D::ResolveDynamicPairVelocities(
        Array<DynamicBody3D>& bodies,
        const float substepDt,
        const PhysicsWorld3DSettings& settings) noexcept {
    for (std::size_t a = 0; a < bodies.GetSize(); ++a) {
        for (std::size_t b = a + 1; b < bodies.GetSize(); ++b) {
            DynamicBody3D& bodyA = bodies[a];
            DynamicBody3D& bodyB = bodies[b];
            RefreshDynamicBody3D(bodyA);
            RefreshDynamicBody3D(bodyB);

            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            if (!bodyA.collider.ComputeDynamicContact(bodyB.collider, nx, ny, nz, pen) || pen <= 0.0F) {
                continue;
            }

            Vector3 va = bodyA.rb->GetVelocity();
            Vector3 vb = bodyB.rb->GetVelocity();
            Vector3 omegaA = bodyA.rb->GetAngularVelocity();
            Vector3 omegaB = bodyB.rb->GetAngularVelocity();
            const float wA = bodyA.rb->GetInverseMass();
            const float wB = bodyB.rb->GetInverseMass();
            const float invIA = bodyA.collider.EffectiveInverseInertia(*bodyA.rb);
            const float invIB = bodyB.collider.EffectiveInverseInertia(*bodyB.rb);
            const float e = PairRestitutionDynamicDynamic(*bodyA.rb, bodyA.obj, *bodyB.rb, bodyB.obj);
            const float mu = PairFrictionDynamicDynamic(bodyA.obj, bodyB.obj);

            ApplyDynamicPairVelocityImpulses(
                    va,
                    vb,
                    &omegaA,
                    invIA,
                    wA,
                    bodyA.collider,
                    &omegaB,
                    invIB,
                    wB,
                    bodyB.collider,
                    nx,
                    ny,
                    nz,
                    e,
                    mu,
                    settings.frictionImpactNormalSpeed,
                    substepDt,
                    settings,
                    pen);
            bodyA.rb->SetVelocity(va);
            bodyB.rb->SetVelocity(vb);
            bodyA.rb->SetAngularVelocity(omegaA);
            bodyB.rb->SetAngularVelocity(omegaB);
        }
    }
}

}  // namespace Spark
