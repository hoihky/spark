#pragma once

#include "spark/math/Constants.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

/**
 * Plane n·x + d = 0 with unit normal n (when normalized).
 */
struct Plane {
    Vector3 normal{Vector3::UnitY};
    float distance = 0.0F;  // offset along normal from origin

    Plane() = default;
    Plane(const Vector3& inNormal, float inDistance) noexcept : normal(inNormal), distance(inDistance) {}

    static Plane FromPointNormal(const Vector3& pointOnPlane, const Vector3& inNormal) noexcept {
        const Vector3 n = inNormal.Normalized();
        return {n, -Vector3::Dot(n, pointOnPlane)};
    }

    /** Creates plane through three CCW points (when viewed from outside along normal). */
    static Plane FromTriangle(const Vector3& a, const Vector3& b, const Vector3& c) noexcept {
        const Vector3 n = Vector3::Cross(b - a, c - a).Normalized();
        return FromPointNormal(a, n);
    }

    [[nodiscard]] Plane Normalized() const noexcept {
        const float len = normal.Length();
        if (len < Epsilon) {
            return *this;
        }
        const float inv = 1.0F / len;
        return {normal * inv, distance * inv};
    }

    /** Signed distance from point to plane (positive = in front of normal direction). */
    [[nodiscard]] float SignedDistanceTo(const Vector3& point) const noexcept {
        return Vector3::Dot(normal, point) + distance;
    }

    [[nodiscard]] bool IsPointOnOrFront(const Vector3& point, float thickness = Epsilon) const noexcept {
        return SignedDistanceTo(point) >= -thickness;
    }

    /** Ray-plane intersection; returns true if hit with t >= 0 (direction need not be unit). */
    bool IntersectRay(const Vector3& rayOrigin, const Vector3& rayDirection, float& outT) const noexcept {
        const float denom = Vector3::Dot(normal, rayDirection);
        if (std::fabs(denom) < Epsilon) {
            return false;
        }
        const float t = -(Vector3::Dot(normal, rayOrigin) + distance) / denom;
        if (t < 0.0F) {
            return false;
        }
        outT = t;
        return true;
    }
};

}  // namespace Spark
