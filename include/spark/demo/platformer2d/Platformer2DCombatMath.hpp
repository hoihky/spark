#pragma once

#include <cmath>

namespace Spark::Platformer2D {

/** Small pure-math helpers shared by combat subsystems (no engine coupling). */
struct CombatMath final {
    CombatMath() = delete;

    [[nodiscard]] static bool BoxOverlap(
            float ax,
            float ay,
            float ahx,
            float ahy,
            float bx,
            float by,
            float bhx,
            float bhy) noexcept
    {
        return std::fabs(ax - bx) <= ahx + bhx && std::fabs(ay - by) <= ahy + bhy;
    }

    /** Writes a unit direction into @p outX/@p outY; falls back to +X when degenerate. */
    static void NormalizeOrDefault(float dx, float dy, float defaultX, float defaultY, float& outX, float& outY) noexcept
    {
        const float len2 = dx * dx + dy * dy;
        if (len2 < 1.0e-8F) {
            outX = defaultX;
            outY = defaultY;
            return;
        }
        const float inv = 1.0F / std::sqrt(len2);
        outX = dx * inv;
        outY = dy * inv;
    }
};

}  // namespace Spark::Platformer2D
