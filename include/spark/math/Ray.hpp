#pragma once

#include "spark/math/Constants.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

/**
 * Semi-infinite ray: origin + t * direction (t >= 0). Direction should be normalized for distance semantics.
 */
struct Ray {
    Vector3 origin{Vector3::Zero};
    Vector3 direction{Vector3::UnitZ};

    Ray() = default;
    Ray(const Vector3& inOrigin, const Vector3& inDirection) noexcept : origin(inOrigin), direction(inDirection) {}

    /** Point at distance t along the ray (not necessarily unit direction — t is in direction length units). */
    [[nodiscard]] Vector3 PointAt(float t) const noexcept { return origin + direction * t; }

    /** Normalizes direction in place; returns false if direction length ~ 0. */
    bool NormalizeDirection() noexcept {
        const float len = direction.Length();
        if (len < Epsilon) {
            return false;
        }
        direction = direction / len;
        return true;
    }

    /** Closest point on ray to arbitrary point (uses normalized direction for true distance along ray). */
    [[nodiscard]] Vector3 ClosestPointTo(const Vector3& point) const noexcept {
        const float lenSq = direction.LengthSquared();
        if (lenSq < Epsilon * Epsilon) {
            return origin;
        }
        const float t = Vector3::Dot(point - origin, direction) / lenSq;
        if (t <= 0.0F) {
            return origin;
        }
        return origin + direction * t;
    }

    /** Squared distance from point to ray segment [origin, +inf). */
    [[nodiscard]] float DistanceSquaredToPoint(const Vector3& point) const noexcept {
        const Vector3 closest = ClosestPointTo(point);
        return (point - closest).LengthSquared();
    }
};

}  // namespace Spark
