#pragma once

namespace Spark {

struct StaticCollider2D;
struct StaticCollider3DSim;

/**
 * Surface material properties for contact resolution (restitution + friction).
 * When <c>isDefined</c> is false, simulators use legacy per-shape defaults.
 */
struct ColliderMaterial {
    bool isDefined = false;
    float restitution = 0.0F;
    float staticFriction = 0.55F;
    float dynamicFriction = 0.48F;

    static ColliderMaterial FromStaticCollider2D(const StaticCollider2D& collider) noexcept;
    static ColliderMaterial FromStaticCollider3D(const StaticCollider3DSim& collider) noexcept;
};

}  // namespace Spark
