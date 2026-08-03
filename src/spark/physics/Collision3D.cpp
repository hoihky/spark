#include "spark/physics/Collision3D.hpp"

#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/MeshCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/shapes/NarrowPhase3D.hpp"
#include "spark/physics/shapes/ShapeFactory3D.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Hp3(const Vector4& p) noexcept {
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

[[nodiscard]] Vector3 LocalCapsuleAxis(const CapsuleDirection3D direction) noexcept {
    switch (direction) {
        case CapsuleDirection3D::X:
            return {1.0F, 0.0F, 0.0F};
        case CapsuleDirection3D::Z:
            return {0.0F, 0.0F, 1.0F};
        case CapsuleDirection3D::Y:
        default:
            return {0.0F, 1.0F, 0.0F};
    }
}

[[nodiscard]] float MaxMatrixScale(const Matrix4& wm) noexcept {
    const float sx = std::sqrt(wm.m[0] * wm.m[0] + wm.m[1] * wm.m[1] + wm.m[2] * wm.m[2]);
    const float sy = std::sqrt(wm.m[4] * wm.m[4] + wm.m[5] * wm.m[5] + wm.m[6] * wm.m[6]);
    const float sz = std::sqrt(wm.m[8] * wm.m[8] + wm.m[9] * wm.m[9] + wm.m[10] * wm.m[10]);
    return std::max({sx, sy, sz});
}

void BuildCapsuleAabbFromEndpoints(
        const Vector3& pointA,
        const Vector3& pointB,
        const float radius,
        CollisionAabb3& outWorld) noexcept {
    outWorld.minX = std::min(pointA.x, pointB.x) - radius;
    outWorld.minY = std::min(pointA.y, pointB.y) - radius;
    outWorld.minZ = std::min(pointA.z, pointB.z) - radius;
    outWorld.maxX = std::max(pointA.x, pointB.x) + radius;
    outWorld.maxY = std::max(pointA.y, pointB.y) + radius;
    outWorld.maxZ = std::max(pointA.z, pointB.z) + radius;
}

[[nodiscard]] bool ClosestPointOnSegment(
        const Vector3& point,
        const Vector3& segA,
        const Vector3& segB,
        Vector3& outClosest,
        float& outT) noexcept {
    const float abx = segB.x - segA.x;
    const float aby = segB.y - segA.y;
    const float abz = segB.z - segA.z;
    const float abLenSq = abx * abx + aby * aby + abz * abz;
    if (abLenSq < 1.0e-12F) {
        outClosest = segA;
        outT = 0.0F;
        return false;
    }
    const float apx = point.x - segA.x;
    const float apy = point.y - segA.y;
    const float apz = point.z - segA.z;
    outT = std::clamp((apx * abx + apy * aby + apz * abz) / abLenSq, 0.0F, 1.0F);
    outClosest.x = segA.x + abx * outT;
    outClosest.y = segA.y + aby * outT;
    outClosest.z = segA.z + abz * outT;
    return true;
}

[[nodiscard]] Vector3 ClosestPointOnAabb(const Vector3& point, const CollisionAabb3& box) noexcept {
    return {
            std::clamp(point.x, box.minX, box.maxX),
            std::clamp(point.y, box.minY, box.maxY),
            std::clamp(point.z, box.minZ, box.maxZ)};
}

void ClosestPointsSegmentAabb(
        const Vector3& segA,
        const Vector3& segB,
        const CollisionAabb3& box,
        Vector3& outOnSegment,
        Vector3& outOnBox) noexcept {
    float bestDistSq = 1.0e30F;
    outOnSegment = segA;
    outOnBox = ClosestPointOnAabb(segA, box);

    auto consider = [&](const float t) noexcept {
        const float u = std::clamp(t, 0.0F, 1.0F);
        const Vector3 p{
                segA.x + (segB.x - segA.x) * u,
                segA.y + (segB.y - segA.y) * u,
                segA.z + (segB.z - segA.z) * u};
        const Vector3 q = ClosestPointOnAabb(p, box);
        const float dx = p.x - q.x;
        const float dy = p.y - q.y;
        const float dz = p.z - q.z;
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < bestDistSq) {
            bestDistSq = d2;
            outOnSegment = p;
            outOnBox = q;
        }
    };

    consider(0.0F);
    consider(1.0F);
    const Vector3 boxCenter{
            0.5F * (box.minX + box.maxX), 0.5F * (box.minY + box.maxY), 0.5F * (box.minZ + box.maxZ)};
    Vector3 segClosest{};
    float segT = 0.0F;
    (void)ClosestPointOnSegment(boxCenter, segA, segB, segClosest, segT);
    consider(segT);

    constexpr int kSamples = 16;
    for (int i = 1; i < kSamples; ++i) {
        consider(static_cast<float>(i) / static_cast<float>(kSamples));
    }
}

void ClosestPointsSegmentSegment(
        const Vector3& p1,
        const Vector3& q1,
        const Vector3& p2,
        const Vector3& q2,
        Vector3& outC1,
        Vector3& outC2) noexcept {
    const float d1x = q1.x - p1.x;
    const float d1y = q1.y - p1.y;
    const float d1z = q1.z - p1.z;
    const float d2x = q2.x - p2.x;
    const float d2y = q2.y - p2.y;
    const float d2z = q2.z - p2.z;
    const float rx = p1.x - p2.x;
    const float ry = p1.y - p2.y;
    const float rz = p1.z - p2.z;

    const float a = d1x * d1x + d1y * d1y + d1z * d1z;
    const float e = d2x * d2x + d2y * d2y + d2z * d2z;
    const float f = d2x * rx + d2y * ry + d2z * rz;

    float s = 0.0F;
    float t = 0.0F;
    if (a <= 1.0e-12F && e <= 1.0e-12F) {
        outC1 = p1;
        outC2 = p2;
        return;
    }
    if (a <= 1.0e-12F) {
        s = 0.0F;
        t = std::clamp(f / e, 0.0F, 1.0F);
    } else {
        const float c = d1x * rx + d1y * ry + d1z * rz;
        if (e <= 1.0e-12F) {
            t = 0.0F;
            s = std::clamp(-c / a, 0.0F, 1.0F);
        } else {
            const float b = d1x * d2x + d1y * d2y + d1z * d2z;
            const float denom = a * e - b * b;
            s = (denom > 1.0e-12F) ? std::clamp((b * f - c * e) / denom, 0.0F, 1.0F) : 0.0F;
            t = (b * s + f) / e;
            if (t < 0.0F) {
                t = 0.0F;
                s = std::clamp(-c / a, 0.0F, 1.0F);
            } else if (t > 1.0F) {
                t = 1.0F;
                s = std::clamp((b - c) / a, 0.0F, 1.0F);
            }
        }
    }

    outC1.x = p1.x + d1x * s;
    outC1.y = p1.y + d1y * s;
    outC1.z = p1.z + d1z * s;
    outC2.x = p2.x + d2x * t;
    outC2.y = p2.y + d2y * t;
    outC2.z = p2.z + d2z * t;
}

}  // namespace

bool CollisionAabb3Overlaps(const CollisionAabb3& a, const CollisionAabb3& b) noexcept {
    return a.minX < b.maxX && a.maxX > b.minX && a.minY < b.maxY && a.maxY > b.minY && a.minZ < b.maxZ &&
            a.maxZ > b.minZ;
}

bool CollisionAabb3OverlapsSphere(const CollisionAabb3& a, const Vector3& center, const float radius) noexcept {
    const float qx = std::clamp(center.x, a.minX, a.maxX);
    const float qy = std::clamp(center.y, a.minY, a.maxY);
    const float qz = std::clamp(center.z, a.minZ, a.maxZ);
    const float dx = center.x - qx;
    const float dy = center.y - qy;
    const float dz = center.z - qz;
    const float rr = radius * radius;
    return dx * dx + dy * dy + dz * dz <= rr + 1.0e-8F;
}

bool CollisionAabb3OverlapsSphereInflated(
        const CollisionAabb3& a,
        const Vector3& center,
        const float baseRadius,
        const float inflateRadius) noexcept {
    const float r = baseRadius + std::max(0.0F, inflateRadius);
    const float qx = std::clamp(center.x, a.minX, a.maxX);
    const float qy = std::clamp(center.y, a.minY, a.maxY);
    const float qz = std::clamp(center.z, a.minZ, a.maxZ);
    const float dx = center.x - qx;
    const float dy = center.y - qy;
    const float dz = center.z - qz;
    const float rr = r * r;
    return dx * dx + dy * dy + dz * dz <= rr + 1.0e-8F;
}

void ComputeBoxCollider3WorldAabb(
        GameObject& owner,
        const BoxCollider3DComponent& collider,
        CollisionAabb3& outWorld) noexcept {
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector3 off = collider.GetOffset();
    const Vector3 he = collider.GetHalfExtents();
    const float x0 = off.x - he.x;
    const float y0 = off.y - he.y;
    const float z0 = off.z - he.z;
    const float x1 = off.x + he.x;
    const float y1 = off.y + he.y;
    const float z1 = off.z + he.z;
    const Vector4 corners[8] = {
            wm * Vector4(x0, y0, z0, 1.0F),
            wm * Vector4(x1, y0, z0, 1.0F),
            wm * Vector4(x1, y1, z0, 1.0F),
            wm * Vector4(x0, y1, z0, 1.0F),
            wm * Vector4(x0, y0, z1, 1.0F),
            wm * Vector4(x1, y0, z1, 1.0F),
            wm * Vector4(x1, y1, z1, 1.0F),
            wm * Vector4(x0, y1, z1, 1.0F),
    };
    Vector3 v0 = Hp3(corners[0]);
    float minX = v0.x;
    float maxX = v0.x;
    float minY = v0.y;
    float maxY = v0.y;
    float minZ = v0.z;
    float maxZ = v0.z;
    for (int i = 1; i < 8; ++i) {
        const Vector3 v = Hp3(corners[static_cast<std::size_t>(i)]);
        minX = std::min(minX, v.x);
        maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
        minZ = std::min(minZ, v.z);
        maxZ = std::max(maxZ, v.z);
    }
    outWorld.minX = minX;
    outWorld.maxX = maxX;
    outWorld.minY = minY;
    outWorld.maxY = maxY;
    outWorld.minZ = minZ;
    outWorld.maxZ = maxZ;
}

void ComputeSphereCollider3World(
        GameObject& owner,
        const SphereCollider3DComponent& collider,
        Vector3& outCenter,
        float& outRadius) noexcept {
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector3 off = collider.GetOffset();
    const Vector4 pc = wm * Vector4(off.x, off.y, off.z, 1.0F);
    outCenter = Hp3(pc);
    const float sx = std::sqrt(wm.m[0] * wm.m[0] + wm.m[1] * wm.m[1] + wm.m[2] * wm.m[2]);
    const float sy = std::sqrt(wm.m[4] * wm.m[4] + wm.m[5] * wm.m[5] + wm.m[6] * wm.m[6]);
    const float sz = std::sqrt(wm.m[8] * wm.m[8] + wm.m[9] * wm.m[9] + wm.m[10] * wm.m[10]);
    const float scale = std::max({sx, sy, sz});
    outRadius = collider.GetRadius() * scale;
}

void ComputeCapsuleCollider3World(
        GameObject& owner,
        const CapsuleCollider3DComponent& collider,
        CollisionCapsule3& outWorld) noexcept {
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector3 localCenter = collider.GetOffset();
    const float localRadius = collider.GetRadius();
    const float localHeight = collider.GetHeight();
    const Vector3 localAxis = LocalCapsuleAxis(collider.GetDirection());

    const float halfTotal = localHeight * 0.5F;
    float segmentHalf = halfTotal - localRadius;
    if (segmentHalf < 0.0F) {
        segmentHalf = 0.0F;
    }

    const Vector3 localA{
            localCenter.x - localAxis.x * segmentHalf,
            localCenter.y - localAxis.y * segmentHalf,
            localCenter.z - localAxis.z * segmentHalf};
    const Vector3 localB{
            localCenter.x + localAxis.x * segmentHalf,
            localCenter.y + localAxis.y * segmentHalf,
            localCenter.z + localAxis.z * segmentHalf};

    outWorld.pointA = Hp3(wm * Vector4(localA.x, localA.y, localA.z, 1.0F));
    outWorld.pointB = Hp3(wm * Vector4(localB.x, localB.y, localB.z, 1.0F));

    const float scale = MaxMatrixScale(wm);
    if (localHeight < localRadius * 2.0F) {
        outWorld.radius = halfTotal * scale;
        const Vector3 center = Hp3(wm * Vector4(localCenter.x, localCenter.y, localCenter.z, 1.0F));
        outWorld.pointA = center;
        outWorld.pointB = center;
    } else {
        outWorld.radius = localRadius * scale;
    }
}

void ComputeCapsuleCollider3WorldAabb(
        GameObject& owner,
        const CapsuleCollider3DComponent& collider,
        CollisionAabb3& outWorld) noexcept {
    CollisionCapsule3 capsule{};
    ComputeCapsuleCollider3World(owner, collider, capsule);
    BuildCapsuleAabbFromEndpoints(capsule.pointA, capsule.pointB, capsule.radius, outWorld);
}

bool ComputeSphereAabbContact(
        const Vector3& pos,
        const float r,
        const CollisionAabb3& b,
        float& outNx,
        float& outNy,
        float& outNz,
        float& outPen,
        const float separationSlopForInclusion) noexcept {
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

bool ComputeSphereCapsuleContact(
        const Vector3& center,
        const float radius,
        const CollisionCapsule3& capsule,
        float& outNx,
        float& outNy,
        float& outNz,
        float& outPen,
        const float separationSlopForInclusion) noexcept {
    Vector3 closest{};
    float t = 0.0F;
    (void)ClosestPointOnSegment(center, capsule.pointA, capsule.pointB, closest, t);
    (void)t;

    const float dx = center.x - closest.x;
    const float dy = center.y - closest.y;
    const float dz = center.z - closest.z;
    const float d2 = dx * dx + dy * dy + dz * dz;
    const float combinedRadius = radius + capsule.radius + std::max(0.0F, separationSlopForInclusion);
    const float penReject =
            (separationSlopForInclusion > 0.0F) ? -(separationSlopForInclusion + 0.001F) : -1.0e-3F;

    constexpr float kEps = 1.0e-8F;
    if (d2 > kEps * kEps) {
        const float d = std::sqrt(d2);
        float pen = combinedRadius - d;
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

    /** Sphere center lies on the capsule segment interior: push along the shortest axis away from the segment. */
    const float abx = capsule.pointB.x - capsule.pointA.x;
    const float aby = capsule.pointB.y - capsule.pointA.y;
    const float abz = capsule.pointB.z - capsule.pointA.z;
    const float abLenSq = abx * abx + aby * aby + abz * abz;
    if (abLenSq < kEps * kEps) {
        outNx = 0.0F;
        outNy = 1.0F;
        outNz = 0.0F;
        outPen = std::max(0.0F, combinedRadius);
        return combinedRadius > penReject;
    }

    const float apx = center.x - capsule.pointA.x;
    const float apy = center.y - capsule.pointA.y;
    const float apz = center.z - capsule.pointA.z;
    float rx = apx - abx * ((apx * abx + apy * aby + apz * abz) / abLenSq);
    float ry = apy - aby * ((apx * abx + apy * aby + apz * abz) / abLenSq);
    float rz = apz - abz * ((apx * abx + apy * aby + apz * abz) / abLenSq);
    const float r2 = rx * rx + ry * ry + rz * rz;
    if (r2 < kEps * kEps) {
        outNx = 0.0F;
        outNy = 1.0F;
        outNz = 0.0F;
    } else {
        const float rLen = std::sqrt(r2);
        outNx = rx / rLen;
        outNy = ry / rLen;
        outNz = rz / rLen;
    }
    outPen = std::max(0.0F, combinedRadius);
    return combinedRadius > penReject;
}

bool ComputeCapsuleAabbContact(
        const CollisionCapsule3& capsule,
        const CollisionAabb3& box,
        float& outNx,
        float& outNy,
        float& outNz,
        float& outPen,
        const float separationSlopForInclusion) noexcept {
    Vector3 onSeg{};
    Vector3 onBox{};
    ClosestPointsSegmentAabb(capsule.pointA, capsule.pointB, box, onSeg, onBox);

    const float dx = onSeg.x - onBox.x;
    const float dy = onSeg.y - onBox.y;
    const float dz = onSeg.z - onBox.z;
    const float d2 = dx * dx + dy * dy + dz * dz;
    const float penReject =
            (separationSlopForInclusion > 0.0F) ? -(separationSlopForInclusion + 0.001F) : -1.0e-3F;
    const float inflatedRadius = capsule.radius + std::max(0.0F, separationSlopForInclusion);

    constexpr float kEps = 1.0e-8F;
    if (d2 > kEps * kEps) {
        const float d = std::sqrt(d2);
        float pen = inflatedRadius - d;
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

    outNx = 0.0F;
    outNy = 1.0F;
    outNz = 0.0F;
    outPen = std::max(0.0F, inflatedRadius);
    return inflatedRadius > penReject;
}

bool ComputeCapsuleCapsuleContact(
        const CollisionCapsule3& a,
        const CollisionCapsule3& b,
        float& outNx,
        float& outNy,
        float& outNz,
        float& outPen,
        const float separationSlopForInclusion) noexcept {
    Vector3 c1{};
    Vector3 c2{};
    ClosestPointsSegmentSegment(a.pointA, a.pointB, b.pointA, b.pointB, c1, c2);

    const float dx = c2.x - c1.x;
    const float dy = c2.y - c1.y;
    const float dz = c2.z - c1.z;
    const float d2 = dx * dx + dy * dy + dz * dz;
    const float combinedRadius = a.radius + b.radius + std::max(0.0F, separationSlopForInclusion);
    const float penReject =
            (separationSlopForInclusion > 0.0F) ? -(separationSlopForInclusion + 0.001F) : -1.0e-3F;

    constexpr float kEps = 1.0e-8F;
    if (d2 > kEps * kEps) {
        const float d = std::sqrt(d2);
        float pen = combinedRadius - d;
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

    outNx = 0.0F;
    outNy = 1.0F;
    outNz = 0.0F;
    outPen = std::max(0.0F, combinedRadius);
    return combinedRadius > penReject;
}

bool ComputeSphereStaticCollider3Contact(
        const Vector3& center,
        const float radius,
        const StaticCollider3DSim& collider,
        float& outNx,
        float& outNy,
        float& outNz,
        float& outPen,
        const float separationSlopForInclusion) noexcept {
    if (collider.shape == StaticCollider3DShape::Capsule) {
        return ComputeSphereCapsuleContact(
                center, radius, collider.capsule, outNx, outNy, outNz, outPen, separationSlopForInclusion);
    }
    return ComputeSphereAabbContact(
            center, radius, collider.aabb, outNx, outNy, outNz, outPen, separationSlopForInclusion);
}

bool StaticCollider3DOverlapsSphere(
        const StaticCollider3DSim& collider,
        const Vector3& center,
        const float radius) noexcept {
    return Collider3D::FromLegacySnapshot(collider).OverlapsSphere(center, radius);
}

bool Collider3DOverlapsSphere(const Collider3D& collider, const Vector3& center, const float radius) noexcept {
    return collider.OverlapsSphere(center, radius);
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

bool ComputeSphereStaticCollider3Contact(
        const Vector3& center,
        const float radius,
        const Collider3D& collider,
        float& outNx,
        float& outNy,
        float& outNz,
        float& outPen,
        const float separationSlopForInclusion) noexcept {
    return ComputeSphereStaticCollider3Contact(
            center, radius, collider.ToLegacySnapshot(), outNx, outNy, outNz, outPen, separationSlopForInclusion);
}

bool SeparateSphereFromStaticCollider3(
        Vector3& center,
        const float radius,
        const Collider3D& collider) noexcept {
    return SeparateSphereFromStaticCollider3(center, radius, collider.ToLegacySnapshot());
}

bool SeparateSphereFromStaticCollider3(
        Vector3& center,
        const float radius,
        const StaticCollider3DSim& collider) noexcept {
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;
    if (!ComputeSphereStaticCollider3Contact(center, radius, collider, nx, ny, nz, pen)) {
        return false;
    }
    if (pen > 1.0e-8F) {
        center.x += nx * pen;
        center.y += ny * pen;
        center.z += nz * pen;
    }
    return true;
}

bool ContributesStaticCollider3D(GameObject& object) noexcept {
    if (object.GetComponent<CharacterController3DComponent>() != nullptr) {
        return false;
    }
    const bool hasBox = object.GetComponent<BoxCollider3DComponent>() != nullptr;
    const bool hasCapsule = object.GetComponent<CapsuleCollider3DComponent>() != nullptr;
    const bool hasMesh = object.GetComponent<MeshCollider3DComponent>() != nullptr;
    if (!hasBox && !hasCapsule && !hasMesh) {
        return false;
    }
    const Rigidbody3DComponent* rb = object.GetComponent<Rigidbody3DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType3D::Dynamic;
}

}  // namespace Spark
