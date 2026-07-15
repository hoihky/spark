#pragma once

#include "spark/math/Constants.hpp"
#include "spark/math/Vector3.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Spark {

/**
 * Unit quaternion (x, y, z, w). Rotation acts as q * v * q* on pure quaternions.
 * Composition: qTotal = qAfter * qBefore applies qBefore first, then qAfter (Hamilton product).
 */
struct Quaternion {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;

    static const Quaternion Identity;

    constexpr Quaternion() noexcept = default;
    constexpr Quaternion(float inX, float inY, float inZ, float inW) noexcept : x(inX), y(inY), z(inZ), w(inW) {}

    static Quaternion FromAxisAngle(const Vector3& axis, float angleRadians) noexcept {
        const Vector3 n = axis.Normalized();
        const float half = angleRadians * 0.5F;
        const float s = std::sin(half);
        const float c = std::cos(half);
        return {n.x * s, n.y * s, n.z * s, c};
    }

    /**
     * Shortest rotation taking unit direction `from` to `to` (glTF mesh +Y → world +Y after skin-node bake).
     */
    [[nodiscard]] static Quaternion FromShortestArc(const Vector3& from, const Vector3& to) noexcept {
        const Vector3 u = from.Normalized();
        const Vector3 v = to.Normalized();
        const float d = std::clamp(Vector3::Dot(u, v), -1.0F, 1.0F);
        if (d > 1.0F - 1.0e-5F) {
            return Identity;
        }
        if (d < -1.0F + 1.0e-5F) {
            const Vector3 ortho = (std::fabs(u.x) < 0.9F) ? Vector3::UnitX : Vector3::UnitZ;
            const Vector3 axis = Vector3::Cross(u, ortho).Normalized();
            return FromAxisAngle(axis, Pi);
        }
        const Vector3 axis = Vector3::Cross(u, v).Normalized();
        return FromAxisAngle(axis, std::acos(d));
    }

    [[nodiscard]] float LengthSquared() const noexcept { return x * x + y * y + z * z + w * w; }
    [[nodiscard]] float Length() const noexcept { return std::sqrt(LengthSquared()); }

    [[nodiscard]] Quaternion Normalized() const noexcept {
        const float len = Length();
        if (len < Epsilon) {
            return Identity;
        }
        const float inv = 1.0F / len;
        return {x * inv, y * inv, z * inv, w * inv};
    }

    [[nodiscard]] Quaternion Conjugate() const noexcept { return {-x, -y, -z, w}; }

    [[nodiscard]] Quaternion operator*(const Quaternion& q) const noexcept {
        return {w * q.x + x * q.w + y * q.z - z * q.y,
                w * q.y - x * q.z + y * q.w + z * q.x,
                w * q.z + x * q.y - y * q.x + z * q.w,
                w * q.w - x * q.x - y * q.y - z * q.z};
    }

    /** Rotates vector v by this quaternion (must be unit for correct rotation). */
    [[nodiscard]] Vector3 RotateVector(const Vector3& v) const noexcept {
        const Quaternion p{v.x, v.y, v.z, 0.0F};
        const Quaternion qn = Normalized();
        const Quaternion r = qn * p * qn.Conjugate();
        return {r.x, r.y, r.z};
    }

    [[nodiscard]] static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) noexcept {
        float cosHalf = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        Quaternion end = b;
        if (cosHalf < 0.0F) {
            cosHalf = -cosHalf;
            end = {-b.x, -b.y, -b.z, -b.w};
        }
        if (cosHalf > 1.0F - Epsilon) {
            return Quaternion{a.x + (end.x - a.x) * t, a.y + (end.y - a.y) * t, a.z + (end.z - a.z) * t,
                              a.w + (end.w - a.w) * t}
                    .Normalized();
        }
        const float halfTheta = std::acos(cosHalf);
        const float sinHalf = std::sin(halfTheta);
        const float ra = std::sin((1.0F - t) * halfTheta) / sinHalf;
        const float rb = std::sin(t * halfTheta) / sinHalf;
        return {a.x * ra + end.x * rb, a.y * ra + end.y * rb, a.z * ra + end.z * rb, a.w * ra + end.w * rb};
    }
};

inline constexpr Quaternion Quaternion::Identity{0.0F, 0.0F, 0.0F, 1.0F};

}  // namespace Spark
