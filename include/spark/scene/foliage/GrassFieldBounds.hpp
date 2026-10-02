#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

/** Axis-aligned coverage on world XZ for a grass field (F2-01). */
class GrassFieldBounds {
public:
    void SetHalfExtentsMeters(float halfX, float halfZ) noexcept;

    [[nodiscard]] float GetHalfExtentX() const noexcept { return halfExtentX; }
    [[nodiscard]] float GetHalfExtentZ() const noexcept { return halfExtentZ; }

    [[nodiscard]] bool ContainsWorldXZ(const Vector3& fieldOriginWorld, float worldX, float worldZ) const noexcept;

private:
    float halfExtentX = 64.0F;
    float halfExtentZ = 64.0F;
};

}  // namespace Spark
