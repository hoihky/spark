#pragma once

#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/** Narrow-phase contact data for a single 2D touch point. */
struct ContactManifold2D {
    /** Unit normal pointing from A toward B (separation direction). */
    Vector2 normal{Vector2::Zero};
    /** World-space contact point (midpoint approximation). */
    Vector2 point{Vector2::Zero};
    /** Positive overlap depth along <c>normal</c>. */
    float penetration = 0.0F;

    [[nodiscard]] bool HasContact() const noexcept { return penetration > 0.0F; }

    void Clear() noexcept {
        normal = Vector2::Zero;
        point = Vector2::Zero;
        penetration = 0.0F;
    }
};

/** Narrow-phase contact data for a single 3D touch point. */
struct ContactManifold3D {
    Vector3 normal{Vector3::Zero};
    Vector3 point{Vector3::Zero};
    float penetration = 0.0F;

    [[nodiscard]] bool HasContact() const noexcept { return penetration > 0.0F; }

    void Clear() noexcept {
        normal = Vector3::Zero;
        point = Vector3::Zero;
        penetration = 0.0F;
    }
};

}  // namespace Spark
