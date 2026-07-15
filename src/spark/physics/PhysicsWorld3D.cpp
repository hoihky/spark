#include "spark/physics/PhysicsWorld3D.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/DistanceJoint3DComponent.hpp"
#include "spark/ecs/components/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/components/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Spark {

namespace {

struct DynSphere {
    GameObject* obj = nullptr;
    Rigidbody3DComponent* rb = nullptr;
    TransformComponent* tr = nullptr;
    SphereCollider3DComponent* sph = nullptr;
};

[[nodiscard]] float EffectiveSphereInverseInertia(const Rigidbody3DComponent& rb, const float radius) noexcept;

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

[[nodiscard]] float PairRestitutionSphereSphere(const Rigidbody3DComponent& a, const GameObject* oa, const Rigidbody3DComponent& b, const GameObject* ob) noexcept {
    const float sa = SurfaceRestitutionOrNeutral(oa);
    const float sb = SurfaceRestitutionOrNeutral(ob);
    const float surface = CombineGeometricMean(sa, sb);
    const float body = CombineGeometricMean(Clamp01(a.GetRestitution()), Clamp01(b.GetRestitution()));
    return Clamp01(surface * body);
}

[[nodiscard]] float PairFrictionSphereSphere(const GameObject* oa, const GameObject* ob) noexcept {
    return CombineGeometricMean(SurfaceDynamicFrictionOrDefault(oa), SurfaceDynamicFrictionOrDefault(ob));
}

void ApplyVelocityRestitutionAlongNormal(
        Vector3& v, const float nx, const float ny, const float nz, const float restitution) noexcept {
    const float e = Clamp01(restitution);
    const float vn = v.x * nx + v.y * ny + v.z * nz;
    if (vn >= 0.0F) {
        return;
    }
    /** Newton: post-contact normal speed <c>v'_n = -e v_n</c> for an infinite-mass surface; impulse <c>-(1+e) v_n</c> along outward <c>n</c>. */
    const float impulse = (1.0F + e) * vn;
    v.x -= impulse * nx;
    v.y -= impulse * ny;
    v.z -= impulse * nz;
}

/**
 * Coulomb kinetic friction after the normal impulse: tangential slip magnitude drops by at most
 * <c>μ * |(1+e) v{n}^{-}|</c>, where <c>v{n}^{-}</c> is the pre-impact normal velocity (negative when approaching).
 * Preserves the normal component of <c>v</c> (already restitution-corrected).
 */
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
    const float tLen = std::sqrt(t2);
    /** No slip budget unless this was a real impact (skip resting / projection micro-slides). */
    if (vnApproach >= -std::max(0.0F, minImpactNormalSpeed)) {
        return;
    }
    const float slipCap = muDyn * (1.0F + e) * std::fabs(vnApproach);
    const float newLen = std::max(0.0F, tLen - slipCap);
    const float s = newLen / tLen;
    v.x = nx * vn + tx * s;
    v.y = ny * vn + ty * s;
    v.z = nz * vn + tz * s;
}

[[nodiscard]] bool ComputeSphereAabbContact(
        const Vector3& pos,
        const float r,
        const CollisionAabb3& b,
        float& outNx,
        float& outNy,
        float& outNz,
        float& outPen,
        const float separationSlopForInclusion = 0.0F) noexcept {
    const bool overlapped = (separationSlopForInclusion > 0.0F)
            ? CollisionAabb3OverlapsSphereInflated(b, pos, r, separationSlopForInclusion)
            : CollisionAabb3OverlapsSphere(b, pos, r);
    if (!overlapped) {
        return false;
    }
    const float penReject =
            (separationSlopForInclusion > 0.0F) ? -(separationSlopForInclusion + 0.001F) : -1.0e-3F;
    const float qx = std::clamp(pos.x, b.minX, b.maxX);
    const float qy = std::clamp(pos.y, b.minY, b.maxY);
    const float qz = std::clamp(pos.z, b.minZ, b.maxZ);
    const float dx = pos.x - qx;
    const float dy = pos.y - qy;
    const float dz = pos.z - qz;
    const float d2 = dx * dx + dy * dy + dz * dz;
    constexpr float kEps = 1.0e-8F;
    if (d2 > kEps * kEps) {
        const float d = std::sqrt(d2);
        float pen = r - d;
        if (pen < penReject) {
            return false;
        }
        if (pen < 0.0F) {
            pen = 0.0F;
        }
        const float inv = 1.0F / d;
        outNx = dx * inv;
        outNy = dy * inv;
        outNz = dz * inv;
        outPen = pen;
        return true;
    }
    float best = 1.0e30F;
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    auto consider = [&](const float p, const float sx, const float sy, const float sz) noexcept {
        if (p > 0.0F && p < best) {
            best = p;
            nx = sx;
            ny = sy;
            nz = sz;
        }
    };
    if (pos.x - r < b.minX) {
        consider((b.minX + r) - pos.x, 1.0F, 0.0F, 0.0F);
    }
    if (pos.x + r > b.maxX) {
        consider(pos.x - (b.maxX - r), -1.0F, 0.0F, 0.0F);
    }
    if (pos.y - r < b.minY) {
        consider((b.minY + r) - pos.y, 0.0F, 1.0F, 0.0F);
    }
    if (pos.y + r > b.maxY) {
        consider(pos.y - (b.maxY - r), 0.0F, -1.0F, 0.0F);
    }
    if (pos.z - r < b.minZ) {
        consider((b.minZ + r) - pos.z, 0.0F, 0.0F, 1.0F);
    }
    if (pos.z + r > b.maxZ) {
        consider(pos.z - (b.maxZ - r), 0.0F, 0.0F, -1.0F);
    }
    if (best >= 1.0e29F) {
        return false;
    }
    outNx = nx;
    outNy = ny;
    outNz = nz;
    outPen = best;
    return true;
}

bool SeparateSphereFromAabb(Vector3& pos, const float r, const CollisionAabb3& b) noexcept {
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;
    if (!ComputeSphereAabbContact(pos, r, b, nx, ny, nz, pen)) {
        return false;
    }
    if (pen > 1.0e-8F) {
        pos.x += nx * pen;
        pos.y += ny * pen;
        pos.z += nz * pen;
    }
    return true;
}

void ApplyDeepestStaticNormalImpulseForSphere(
        DynSphere& d,
        const Array<StaticCollider3DSim>& statics,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch,
        const float h,
        const PhysicsWorld3DSettings& settings) noexcept {
    Vector3 center{};
    float rad = 0.0F;
    ComputeSphereCollider3World(*d.obj, *d.sph, center, rad);

    /** Widen query so thin post-separation gaps still find the same statics after float error. */
    constexpr float kBroadInflate = 0.006F;
    CollisionAabb3 q{};
    q.minX = center.x - rad - kBroadInflate;
    q.minY = center.y - rad - kBroadInflate;
    q.minZ = center.z - rad - kBroadInflate;
    q.maxX = center.x + rad + kBroadInflate;
    q.maxY = center.y + rad + kBroadInflate;
    q.maxZ = center.z + rad + kBroadInflate;
    broadPhase.QueryUniquePayloadIndices(q, scratch);

    Vector3 vel = d.rb->GetVelocity();
    Vector3 omega = d.rb->GetAngularVelocity();

    /** Slightly inflated narrow phase: strict overlap often disappears after position solve while velocity still
     *  points into the surface, which skipped impulses entirely ("glued", no bounce). */
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
            if (!ComputeSphereAabbContact(center, rad, rec.aabb, nx, ny, nz, pen, kNarrowSlop)) {
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
        const float ePair = PairRestitution(*d.rb, d.obj, rec);
        const float mu = PairDynamicFriction(d.obj, rec);
        const Vector3 vPhys0 = vel;
        ApplyVelocityRestitutionAlongNormal(vel, bnX, bnY, bnZ, ePair);
        ApplyCoulombKineticFrictionTangent(
                vel, bnX, bnY, bnZ, mu, vn0, ePair, settings.frictionImpactNormalSpeed);

        const float invM = d.rb->GetInverseMass();
        const float invI = EffectiveSphereInverseInertia(*d.rb, rad);
        if (invM > 1.0e-10F && invI > 1.0e-10F) {
            const Vector3 dv{vel.x - vPhys0.x, vel.y - vPhys0.y, vel.z - vPhys0.z};
            const Vector3 impulse{dv.x / invM, dv.y / invM, dv.z / invM};
            const Vector3 rArm{-bnX * rad, -bnY * rad, -bnZ * rad};
            const Vector3 tau = Vector3::Cross(rArm, impulse);
            omega.x += invI * tau.x;
            omega.y += invI * tau.y;
            omega.z += invI * tau.z;
        }

        const bool baumgarteEnable = settings.baumgarteContactBias > 0.0F;
        if (baumgarteEnable && bestPen > settings.contactPenetrationSlop) {
            const float hh = std::max(h, 1.0e-6F);
            const float bias =
                    std::min(settings.baumgarteMaxSeparationVelocity, settings.baumgarteContactBias * bestPen / hh);
            vel.x += bias * bnX;
            vel.y += bias * bnY;
            vel.z += bias * bnZ;
        }
    }

    d.rb->SetVelocity(vel);
    d.rb->SetAngularVelocity(omega);
}

bool SeparateSpheresPosition(
        Vector3& ca,
        Vector3& cb,
        const float wA,
        const float ra,
        const float wB,
        const float rb) noexcept {
    Vector3 d = cb - ca;
    const float distSq = d.LengthSquared();
    const float minD = 1.0e-8F;
    if (distSq < minD * minD) {
        return false;
    }
    const float dist = std::sqrt(distSq);
    const float pen = ra + rb - dist;
    if (pen <= 0.0F) {
        return false;
    }
    const float nx = d.x / dist;
    const float ny = d.y / dist;
    const float nz = d.z / dist;
    const float den = wA + wB;
    if (den < 1.0e-10F) {
        return false;
    }
    const float corr = pen / den;
    ca.x -= nx * corr * wB;
    ca.y -= ny * corr * wB;
    ca.z -= nz * corr * wB;
    cb.x += nx * corr * wA;
    cb.y += ny * corr * wA;
    cb.z += nz * corr * wA;
    return true;
}

void ApplySpherePairVelocityImpulses(
        Vector3& va,
        Vector3& vb,
        Vector3* omegaA,
        const float invIA,
        const float wA,
        const float ra,
        Vector3* omegaB,
        const float invIB,
        const float wB,
        const float rb,
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
        const Vector3 rA{nx * ra, ny * ra, nz * ra};
        const Vector3 tau = Vector3::Cross(rA, impulse);
        omegaA->x += invIA * tau.x;
        omegaA->y += invIA * tau.y;
        omegaA->z += invIA * tau.z;
    }
    if (omegaB != nullptr && invIB > 0.0F && wB > 1.0e-10F) {
        const Vector3 dv{vb.x - vbPhys0.x, vb.y - vbPhys0.y, vb.z - vbPhys0.z};
        const Vector3 impulse{dv.x / wB, dv.y / wB, dv.z / wB};
        const Vector3 rB{-nx * rb, -ny * rb, -nz * rb};
        const Vector3 tau = Vector3::Cross(rB, impulse);
        omegaB->x += invIB * tau.x;
        omegaB->y += invIB * tau.y;
        omegaB->z += invIB * tau.z;
    }

    const bool baumgarteEnable = settings.baumgarteContactBias > 0.0F;
    if (baumgarteEnable && pen > settings.contactPenetrationSlop) {
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

/** Isotropic inverse inertia for a solid sphere: <c>I = 2/5 m r²</c>. */
[[nodiscard]] float SolidSphereInverseInertiaScalar(const float invMass, const float radius) noexcept {
    if (invMass <= 0.0F || radius < 1.0e-6F) {
        return 0.0F;
    }
    const float rr = radius * radius;
    return 2.5F * invMass / rr;
}

[[nodiscard]] float EffectiveSphereInverseInertia(const Rigidbody3DComponent& rb, const float radius) noexcept {
    const float overrideInv = rb.GetInverseInertiaTensorScale();
    if (overrideInv > 0.0F) {
        return overrideInv;
    }
    return SolidSphereInverseInertiaScalar(rb.GetInverseMass(), radius);
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

[[nodiscard]] bool AnyStaticOverlap(
        const Vector3& center,
        const float rad,
        const Array<StaticCollider3DSim>& statics,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    CollisionAabb3 q{};
    q.minX = center.x - rad;
    q.minY = center.y - rad;
    q.minZ = center.z - rad;
    q.maxX = center.x + rad;
    q.maxY = center.y + rad;
    q.maxZ = center.z + rad;
    broadPhase.QueryUniquePayloadIndices(q, scratch);
    for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
        const std::uint32_t idx = scratch[i];
        if (idx >= statics.GetSize()) {
            continue;
        }
        if (CollisionAabb3OverlapsSphere(statics[idx].aabb, center, rad)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] float ComputeTranslationLambdaAgainstStatics(
        const Vector3& c0,
        const Vector3& c1,
        const float rad,
        const int binaryIters,
        const Array<StaticCollider3DSim>& statics,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    if (binaryIters <= 0) {
        return 1.0F;
    }
    if (!AnyStaticOverlap(c1, rad, statics, broadPhase, scratch)) {
        return 1.0F;
    }
    if (!AnyStaticOverlap(c0, rad, statics, broadPhase, scratch)) {
        /** Already separated at start; find latest free fraction along the segment. */
        float lo = 0.0F;
        float hi = 1.0F;
        for (int k = 0; k < binaryIters; ++k) {
            const float mid = 0.5F * (lo + hi);
            const Vector3 cm{c0.x + (c1.x - c0.x) * mid, c0.y + (c1.y - c0.y) * mid, c0.z + (c1.z - c0.z) * mid};
            if (AnyStaticOverlap(cm, rad, statics, broadPhase, scratch)) {
                hi = mid;
            } else {
                lo = mid;
            }
        }
        return std::max(0.0F, lo * 0.999F);
    }
    /** Deep overlap at start: do not shorten the step (positional solve will recover). */
    return 1.0F;
}

void CollectDynamics(GameWorld& world, Array<DynSphere>& out) noexcept {
    out.Clear();
    world.ForEachGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        auto* rb = o->GetComponent<Rigidbody3DComponent>();
        auto* tr = o->GetComponent<TransformComponent>();
        auto* sph = o->GetComponent<SphereCollider3DComponent>();
        if (rb == nullptr || tr == nullptr || sph == nullptr) {
            return;
        }
        if (rb->GetBodyType() != RigidbodyBodyType3D::Dynamic) {
            return;
        }
        DynSphere d{};
        d.obj = o;
        d.rb = rb;
        d.tr = tr;
        d.sph = sph;
        out.PushBack(d);
    });
}

void SolveDistanceJoints(GameWorld& world, const float stiffnessScale) noexcept {
    world.ForEachGameObject([&](GameObject* owner) {
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
    Array<DynSphere> bodies;
    CollectDynamics(world, bodies);

    for (int s = 0; s < substeps; ++s) {
        for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
            DynSphere& d = bodies[bi];
            Vector3 v = d.rb->GetVelocity();
            v.y += settings.gravityY * d.rb->GetGravityScale() * h;
            if (v.y < -settings.maxFallSpeed) {
                v.y = -settings.maxFallSpeed;
            }
            if (v.y > settings.maxFallSpeed) {
                v.y = settings.maxFallSpeed;
            }
            ApplyLinearDamping(v, d.rb->GetLinearDamping(), h);
            d.rb->SetVelocity(v);

            Vector3 w = d.rb->GetAngularVelocity();
            ApplyAngularDamping(w, d.rb->GetAngularDamping(), h);
            d.rb->SetAngularVelocity(w);

            const Vector3 t0 = d.tr->GetLocalTransform().translation;
            Vector3 c0{};
            float rad = 0.0F;
            ComputeSphereCollider3World(*d.obj, *d.sph, c0, rad);
            const Vector3 c1{c0.x + v.x * h, c0.y + v.y * h, c0.z + v.z * h};
            const float lambda = ComputeTranslationLambdaAgainstStatics(
                    c0,
                    c1,
                    rad,
                    settings.sweptStaticCcdBinaryIterations,
                    statics,
                    broadPhase,
                    queryScratch);
            const Vector3 cSafe{
                    c0.x + (c1.x - c0.x) * lambda, c0.y + (c1.y - c0.y) * lambda, c0.z + (c1.z - c0.z) * lambda};
            const Vector3 tNew{
                    t0.x + (cSafe.x - c0.x), t0.y + (cSafe.y - c0.y), t0.z + (cSafe.z - c0.z)};
            d.tr->SetTranslation(tNew);

            IntegrateLocalOrientation(*d.tr, w, h);
        }

        /** Velocity impulses while overlaps are reliable (post-integration). Position iterations may open a float gap
         *  that no longer passes strict AABB–sphere tests, which previously skipped restitution entirely. */
        for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
            ApplyDeepestStaticNormalImpulseForSphere(bodies[bi], statics, broadPhase, queryScratch, h, settings);
        }

        const int iters = std::max(1, settings.resolveIterations);
        for (int it = 0; it < iters; ++it) {
            for (std::size_t bi = 0; bi < bodies.GetSize(); ++bi) {
                DynSphere& d = bodies[bi];
                Vector3 center{};
                float rad = 0.0F;
                ComputeSphereCollider3World(*d.obj, *d.sph, center, rad);

                CollisionAabb3 q{};
                q.minX = center.x - rad;
                q.minY = center.y - rad;
                q.minZ = center.z - rad;
                q.maxX = center.x + rad;
                q.maxY = center.y + rad;
                q.maxZ = center.z + rad;

                broadPhase.QueryUniquePayloadIndices(q, queryScratch);

                Vector3 trPos = d.tr->GetLocalTransform().translation;

                for (std::size_t i = 0; i < queryScratch.GetSize(); ++i) {
                    const std::uint32_t idx = queryScratch[i];
                    if (idx >= statics.GetSize()) {
                        continue;
                    }
                    const StaticCollider3DSim& rec = statics[idx];
                    if (!CollisionAabb3OverlapsSphere(rec.aabb, center, rad)) {
                        continue;
                    }
                    Vector3 c = center;
                    if (SeparateSphereFromAabb(c, rad, rec.aabb)) {
                        trPos += (c - center);
                        d.tr->SetTranslation(trPos);
                        center = c;
                    }
                }
            }

            for (std::size_t a = 0; a < bodies.GetSize(); ++a) {
                for (std::size_t b = a + 1; b < bodies.GetSize(); ++b) {
                    DynSphere& A = bodies[a];
                    DynSphere& B = bodies[b];
                    Vector3 ca{};
                    float ra = 0.0F;
                    Vector3 cb{};
                    float rb = 0.0F;
                    ComputeSphereCollider3World(*A.obj, *A.sph, ca, ra);
                    ComputeSphereCollider3World(*B.obj, *B.sph, cb, rb);

                    const float wA = A.rb->GetInverseMass();
                    const float wB = B.rb->GetInverseMass();

                    const Vector3 ca0 = ca;
                    const Vector3 cb0 = cb;
                    if (SeparateSpheresPosition(ca, cb, wA, ra, wB, rb)) {
                        Vector3 pa = A.tr->GetLocalTransform().translation;
                        Vector3 pb = B.tr->GetLocalTransform().translation;
                        pa += (ca - ca0);
                        pb += (cb - cb0);
                        A.tr->SetTranslation(pa);
                        B.tr->SetTranslation(pb);
                    }
                }
            }
        }

        for (std::size_t a = 0; a < bodies.GetSize(); ++a) {
            for (std::size_t b = a + 1; b < bodies.GetSize(); ++b) {
                DynSphere& A = bodies[a];
                DynSphere& B = bodies[b];
                Vector3 ca{};
                float ra = 0.0F;
                Vector3 cb{};
                float rb = 0.0F;
                ComputeSphereCollider3World(*A.obj, *A.sph, ca, ra);
                ComputeSphereCollider3World(*B.obj, *B.sph, cb, rb);
                Vector3 d = cb - ca;
                const float distSq = d.LengthSquared();
                if (distSq < 1.0e-16F) {
                    continue;
                }
                const float dist = std::sqrt(distSq);
                const float pen = ra + rb - dist;
                if (pen <= 0.0F) {
                    continue;
                }
                const float nx = d.x / dist;
                const float ny = d.y / dist;
                const float nz = d.z / dist;

                Vector3 va = A.rb->GetVelocity();
                Vector3 vb = B.rb->GetVelocity();
                Vector3 omegaA = A.rb->GetAngularVelocity();
                Vector3 omegaB = B.rb->GetAngularVelocity();
                const float wA = A.rb->GetInverseMass();
                const float wB = B.rb->GetInverseMass();
                const float invIA = EffectiveSphereInverseInertia(*A.rb, ra);
                const float invIB = EffectiveSphereInverseInertia(*B.rb, rb);
                const float e = PairRestitutionSphereSphere(*A.rb, A.obj, *B.rb, B.obj);
                const float mu = PairFrictionSphereSphere(A.obj, B.obj);

                ApplySpherePairVelocityImpulses(
                        va,
                        vb,
                        &omegaA,
                        invIA,
                        wA,
                        ra,
                        &omegaB,
                        invIB,
                        wB,
                        rb,
                        nx,
                        ny,
                        nz,
                        e,
                        mu,
                        settings.frictionImpactNormalSpeed,
                        h,
                        settings,
                        pen);
                A.rb->SetVelocity(va);
                B.rb->SetVelocity(vb);
                A.rb->SetAngularVelocity(omegaA);
                B.rb->SetAngularVelocity(omegaB);
            }
        }

        const int jIters = std::max(0, settings.jointIterations);
        for (int j = 0; j < jIters; ++j) {
            SolveDistanceJoints(world, 1.0F / static_cast<float>(std::max(1, jIters)));
        }
    }
}

}  // namespace Spark
