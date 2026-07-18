#include "spark/physics/PhysicsWorld3D.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/DistanceJoint3DComponent.hpp"
#include "spark/ecs/components/physics/3d/HingeJoint3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SpringJoint3DComponent.hpp"
#include "spark/physics/PhysicsJoints.hpp"
#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/DynamicCollider3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Spark {

namespace {

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
        const StaticCollider3DSim& st) noexcept {
    if (!st.hasMaterial) {
        return Clamp01(dynRb.GetRestitution());
    }
    const float geom = CombineGeometricMean(SurfaceRestitutionOrNeutral(dynObj), Clamp01(st.restitution));
    return Clamp01(dynRb.GetRestitution() * geom);
}

[[nodiscard]] float PairDynamicFriction(const GameObject* dynObj, const StaticCollider3DSim& st) noexcept {
    if (!st.hasMaterial) {
        return 0.0F;
    }
    return CombineGeometricMean(SurfaceDynamicFrictionOrDefault(dynObj), std::max(0.0F, st.dynamicFriction));
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

void RefreshDynamicBodySim(DynamicBody3D& body) noexcept {
    if (body.sphere != nullptr) {
        BuildDynamicCollider3DSimFromSphere(*body.obj, *body.sphere, body.sim);
    } else if (body.capsule != nullptr) {
        BuildDynamicCollider3DSimFromCapsule(*body.obj, *body.capsule, body.sim);
    }
}

void ApplyTranslationDeltaToBody(DynamicBody3D& body, const Vector3& delta) noexcept {
    Vector3 translation = body.tr->GetLocalTransform().translation;
    translation.x += delta.x;
    translation.y += delta.y;
    translation.z += delta.z;
    body.tr->SetTranslation(translation);
    TranslateDynamicCollider3DSim(body.sim, delta);
}

void ApplyDeepestStaticNormalImpulse(
        DynamicBody3D& body,
        const Array<StaticCollider3DSim>& statics,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch,
        const float h,
        const PhysicsWorld3DSettings& settings) noexcept {
    RefreshDynamicBodySim(body);

    constexpr float kBroadInflate = 0.006F;
    CollisionAabb3 q = body.sim.bounds;
    q.minX -= kBroadInflate;
    q.minY -= kBroadInflate;
    q.minZ -= kBroadInflate;
    q.maxX += kBroadInflate;
    q.maxY += kBroadInflate;
    q.maxZ += kBroadInflate;
    broadPhase.QueryUniquePayloadIndices(q, scratch);

    Vector3 vel = body.rb->GetVelocity();
    Vector3 omega = body.rb->GetAngularVelocity();
    const Vector3 anchor = GetDynamicCollider3DTrackingPoint(body.sim);

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
            if (idx >= statics.GetSize()) {
                continue;
            }
            const StaticCollider3DSim& rec = statics[idx];
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            if (!ComputeDynamicStaticCollider3Contact(body.sim, rec, nx, ny, nz, pen, kNarrowSlop)) {
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
        const StaticCollider3DSim& rec = statics[bestIdx];
        const float ePair = PairRestitution(*body.rb, body.obj, rec);
        const float mu = PairDynamicFriction(body.obj, rec);
        const Vector3 vPhys0 = vel;
        ApplyVelocityRestitutionAlongNormal(vel, bnX, bnY, bnZ, ePair);
        ApplyCoulombKineticFrictionTangent(
                vel, bnX, bnY, bnZ, mu, vn0, ePair, settings.frictionImpactNormalSpeed);

        const float invM = body.rb->GetInverseMass();
        const float invI = EffectiveDynamicCollider3DInverseInertia(*body.rb, body.sim);
        if (invM > 1.0e-10F && invI > 1.0e-10F) {
            const Vector3 dv{vel.x - vPhys0.x, vel.y - vPhys0.y, vel.z - vPhys0.z};
            const Vector3 impulse{dv.x / invM, dv.y / invM, dv.z / invM};
            Vector3 contactOnBody{
                    anchor.x - bnX * (body.sim.shape == DynamicCollider3DShape::Sphere ? body.sim.sphereRadius
                                                                                      : body.sim.capsule.radius),
                    anchor.y - bnY * (body.sim.shape == DynamicCollider3DShape::Sphere ? body.sim.sphereRadius
                                                                                      : body.sim.capsule.radius),
                    anchor.z - bnZ * (body.sim.shape == DynamicCollider3DShape::Sphere ? body.sim.sphereRadius
                                                                                      : body.sim.capsule.radius)};
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
        const DynamicCollider3DSim& simA,
        Vector3* omegaB,
        const float invIB,
        const float wB,
        const DynamicCollider3DSim& simB,
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
        const Vector3 anchorA = GetDynamicCollider3DTrackingPoint(simA);
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
        const Vector3 anchorB = GetDynamicCollider3DTrackingPoint(simB);
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

[[nodiscard]] bool AnyStaticOverlapForDynamic(
        const DynamicCollider3DSim& sim,
        const Array<StaticCollider3DSim>& statics,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    broadPhase.QueryUniquePayloadIndices(sim.bounds, scratch);
    for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
        const std::uint32_t idx = scratch[i];
        if (idx >= statics.GetSize()) {
            continue;
        }
        if (DynamicCollider3DOverlapsStatic(sim, statics[idx])) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] float ComputeTranslationLambdaAgainstStatics(
        const DynamicCollider3DSim& simAtStart,
        const Vector3& track0,
        const Vector3& track1,
        const int binaryIters,
        const Array<StaticCollider3DSim>& statics,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    if (binaryIters <= 0) {
        return 1.0F;
    }

    DynamicCollider3DSim simEnd = simAtStart;
    const Vector3 endDelta{track1.x - track0.x, track1.y - track0.y, track1.z - track0.z};
    TranslateDynamicCollider3DSim(simEnd, endDelta);
    if (!AnyStaticOverlapForDynamic(simEnd, statics, broadPhase, scratch)) {
        return 1.0F;
    }
    if (!AnyStaticOverlapForDynamic(simAtStart, statics, broadPhase, scratch)) {
        float lo = 0.0F;
        float hi = 1.0F;
        for (int k = 0; k < binaryIters; ++k) {
            const float mid = 0.5F * (lo + hi);
            DynamicCollider3DSim simMid = simAtStart;
            TranslateDynamicCollider3DSim(simMid, {endDelta.x * mid, endDelta.y * mid, endDelta.z * mid});
            if (AnyStaticOverlapForDynamic(simMid, statics, broadPhase, scratch)) {
                hi = mid;
            } else {
                lo = mid;
            }
        }
        return std::max(0.0F, lo * 0.999F);
    }
    return 1.0F;
}

void CollectDynamics(GameWorld& world, Array<DynamicBody3D>& out) noexcept {
    out.Clear();
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        if (o->GetComponent<CharacterController3DComponent>() != nullptr) {
            return;
        }
        auto* rb = o->GetComponent<Rigidbody3DComponent>();
        auto* tr = o->GetComponent<TransformComponent>();
        if (rb == nullptr || tr == nullptr) {
            return;
        }
        if (rb->GetBodyType() != RigidbodyBodyType3D::Dynamic) {
            return;
        }

        auto* sphere = o->GetComponent<SphereCollider3DComponent>();
        auto* capsule = o->GetComponent<CapsuleCollider3DComponent>();
        if (sphere == nullptr && capsule == nullptr) {
            return;
        }

        DynamicBody3D body{};
        body.obj = o;
        body.rb = rb;
        body.tr = tr;
        body.sphere = sphere;
        body.capsule = (sphere == nullptr) ? capsule : nullptr;
        RefreshDynamicBodySim(body);
        out.PushBack(body);
    });
}

void SolveDistanceJoints(GameWorld& world, const float stiffnessScale) noexcept {
    world.ForEachActiveGameObject([&](GameObject* owner) {
        if (owner == nullptr) {
            return;
        }
        auto* joint = owner->GetComponent<DistanceJoint3DComponent>();
        if (joint == nullptr) {
            return;
        }
        GameObject* other = joint->GetConnectedBody();
        if (other == nullptr) {
            return;
        }
        auto* trA = owner->GetComponent<TransformComponent>();
        auto* trB = other->GetComponent<TransformComponent>();
        auto* spA = owner->GetComponent<SphereCollider3DComponent>();
        auto* spB = other->GetComponent<SphereCollider3DComponent>();
        auto* rbA = owner->GetComponent<Rigidbody3DComponent>();
        auto* rbB = other->GetComponent<Rigidbody3DComponent>();
        if (trA == nullptr || trB == nullptr || spA == nullptr || spB == nullptr) {
            return;
        }

        Vector3 ca{};
        float ra = 0.0F;
        Vector3 cb{};
        float rb = 0.0F;
        ComputeSphereCollider3World(*owner, *spA, ca, ra);
        ComputeSphereCollider3World(*other, *spB, cb, rb);

        Vector3 d = cb - ca;
        const float len = d.Length();
        if (len < 1.0e-6F) {
            return;
        }
        const float nx = d.x / len;
        const float ny = d.y / len;
        const float nz = d.z / len;

        const float wA = (rbA != nullptr && rbA->GetBodyType() == RigidbodyBodyType3D::Dynamic) ? rbA->GetInverseMass() : 0.0F;
        const float wB = (rbB != nullptr && rbB->GetBodyType() == RigidbodyBodyType3D::Dynamic) ? rbB->GetInverseMass() : 0.0F;

        const float err = len - joint->GetRestLength();
        const float k = joint->GetStiffness() * stiffnessScale;
        const float correction = err * k;

        if (wA > 1.0e-8F && wB > 1.0e-8F) {
            const float den = wA + wB;
            const float uA = nx * correction * wB / den;
            const float uAy = ny * correction * wB / den;
            const float uAz = nz * correction * wB / den;
            const float uB = -nx * correction * wA / den;
            const float uBy = -ny * correction * wA / den;
            const float uBz = -nz * correction * wA / den;
            Vector3 pa = trA->GetLocalTransform().translation;
            pa.x += uA;
            pa.y += uAy;
            pa.z += uAz;
            trA->SetTranslation(pa);
            Vector3 pb = trB->GetLocalTransform().translation;
            pb.x += uB;
            pb.y += uBy;
            pb.z += uBz;
            trB->SetTranslation(pb);
        } else if (wA > 1.0e-8F) {
            Vector3 pa = trA->GetLocalTransform().translation;
            pa.x += nx * correction;
            pa.y += ny * correction;
            pa.z += nz * correction;
            trA->SetTranslation(pa);
        } else if (wB > 1.0e-8F) {
            Vector3 pb = trB->GetLocalTransform().translation;
            pb.x -= nx * correction;
            pb.y -= ny * correction;
            pb.z -= nz * correction;
            trB->SetTranslation(pb);
        }
    });
}

}  // namespace

void SimulatePhysics3D(GameWorld& world, const FrameTiming& timing, const PhysicsWorld3DSettings& settings) {
    const float dt = timing.deltaTimeSeconds;
    if (dt <= 0.0F) {
        return;
    }

    const int substeps = std::max(1, settings.substeps);
    const float h = dt / static_cast<float>(substeps);

    Array<StaticCollider3DSim> statics;
    SpatialHashGrid3D broadPhase;
    constexpr float kBroadCellWorld = 2.0F;
    RebuildBroadPhaseFromStaticColliders3D(world, kBroadCellWorld, statics, broadPhase);

    Array<std::uint32_t> queryScratch;
    Array<DynamicBody3D> bodies;
    CollectDynamics(world, bodies);

    for (int s = 0; s < substeps; ++s) {
        for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
            DynamicBody3D& body = bodies[bi];
            Vector3 v = body.rb->GetVelocity();
            v.y += settings.gravityY * body.rb->GetGravityScale() * h;
            v.y = std::clamp(v.y, -settings.maxFallSpeed, settings.maxFallSpeed);
            ApplyLinearDamping(v, body.rb->GetLinearDamping(), h);
            body.rb->SetVelocity(v);

            Vector3 w = body.rb->GetAngularVelocity();
            ApplyAngularDamping(w, body.rb->GetAngularDamping(), h);
            body.rb->SetAngularVelocity(w);

            RefreshDynamicBodySim(body);
            const Vector3 track0 = GetDynamicCollider3DTrackingPoint(body.sim);
            const Vector3 track1{track0.x + v.x * h, track0.y + v.y * h, track0.z + v.z * h};
            const float lambda = ComputeTranslationLambdaAgainstStatics(
                    body.sim, track0, track1, settings.sweptStaticCcdBinaryIterations, statics, broadPhase, queryScratch);
            const Vector3 trackSafe{
                    track0.x + (track1.x - track0.x) * lambda,
                    track0.y + (track1.y - track0.y) * lambda,
                    track0.z + (track1.z - track0.z) * lambda};
            ApplyTranslationDeltaToBody(body, {trackSafe.x - track0.x, trackSafe.y - track0.y, trackSafe.z - track0.z});

            IntegrateLocalOrientation(*body.tr, w, h);
        }

        for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
            ApplyDeepestStaticNormalImpulse(bodies[bi], statics, broadPhase, queryScratch, h, settings);
        }

        const int iters = std::max(1, settings.resolveIterations);
        for (int it = 0; it < iters; ++it) {
            for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
                DynamicBody3D& body = bodies[bi];
                RefreshDynamicBodySim(body);

                broadPhase.QueryUniquePayloadIndices(body.sim.bounds, queryScratch);
                for (std::size_t i = 0; i < queryScratch.GetSize(); ++i) {
                    const std::uint32_t idx = queryScratch[i];
                    if (idx >= statics.GetSize()) {
                        continue;
                    }
                    const StaticCollider3DSim& rec = statics[idx];
                    if (!DynamicCollider3DOverlapsStatic(body.sim, rec)) {
                        continue;
                    }
                    const Vector3 trackBefore = GetDynamicCollider3DTrackingPoint(body.sim);
                    DynamicCollider3DSim separated = body.sim;
                    if (SeparateDynamicCollider3DFromStatic(separated, rec)) {
                        const Vector3 trackAfter = GetDynamicCollider3DTrackingPoint(separated);
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
                    RefreshDynamicBodySim(bodyA);
                    RefreshDynamicBodySim(bodyB);

                    const Vector3 trackA0 = GetDynamicCollider3DTrackingPoint(bodyA.sim);
                    const Vector3 trackB0 = GetDynamicCollider3DTrackingPoint(bodyB.sim);
                    DynamicCollider3DSim simA = bodyA.sim;
                    DynamicCollider3DSim simB = bodyB.sim;
                    if (SeparateDynamicDynamicCollider3Position(
                                simA, simB, bodyA.rb->GetInverseMass(), bodyB.rb->GetInverseMass())) {
                        const Vector3 trackA1 = GetDynamicCollider3DTrackingPoint(simA);
                        const Vector3 trackB1 = GetDynamicCollider3DTrackingPoint(simB);
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

        for (std::size_t a = 0; a < bodies.GetSize(); ++a) {
            for (std::size_t b = a + 1; b < bodies.GetSize(); ++b) {
                DynamicBody3D& bodyA = bodies[a];
                DynamicBody3D& bodyB = bodies[b];
                RefreshDynamicBodySim(bodyA);
                RefreshDynamicBodySim(bodyB);

                float nx = 0.0F;
                float ny = 0.0F;
                float nz = 0.0F;
                float pen = 0.0F;
                if (!ComputeDynamicDynamicCollider3Contact(bodyA.sim, bodyB.sim, nx, ny, nz, pen) || pen <= 0.0F) {
                    continue;
                }

                Vector3 va = bodyA.rb->GetVelocity();
                Vector3 vb = bodyB.rb->GetVelocity();
                Vector3 omegaA = bodyA.rb->GetAngularVelocity();
                Vector3 omegaB = bodyB.rb->GetAngularVelocity();
                const float wA = bodyA.rb->GetInverseMass();
                const float wB = bodyB.rb->GetInverseMass();
                const float invIA = EffectiveDynamicCollider3DInverseInertia(*bodyA.rb, bodyA.sim);
                const float invIB = EffectiveDynamicCollider3DInverseInertia(*bodyB.rb, bodyB.sim);
                const float e = PairRestitutionDynamicDynamic(*bodyA.rb, bodyA.obj, *bodyB.rb, bodyB.obj);
                const float mu = PairFrictionDynamicDynamic(bodyA.obj, bodyB.obj);

                ApplyDynamicPairVelocityImpulses(
                        va,
                        vb,
                        &omegaA,
                        invIA,
                        wA,
                        bodyA.sim,
                        &omegaB,
                        invIB,
                        wB,
                        bodyB.sim,
                        nx,
                        ny,
                        nz,
                        e,
                        mu,
                        settings.frictionImpactNormalSpeed,
                        h,
                        settings,
                        pen);
                bodyA.rb->SetVelocity(va);
                bodyB.rb->SetVelocity(vb);
                bodyA.rb->SetAngularVelocity(omegaA);
                bodyB.rb->SetAngularVelocity(omegaB);
            }
        }

        const int jIters = std::max(0, settings.jointIterations);
        for (int j = 0; j < jIters; ++j) {
            const float scale = 1.0F / static_cast<float>(std::max(1, jIters));
            SolveDistanceJoints(world, scale);
            SolveHingeJoints3D(world, scale);
        }
        SolveSpringJoints3D(world, h);
    }
}

}  // namespace Spark
