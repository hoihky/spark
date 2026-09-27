#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"

namespace Spark {

/**
 * Named 2D spawn marker (position from transform, optional planar facing).
 * Resolved through <c>SpawnPoint2DService</c> / <c>FindSpawnPoint2D</c>.
 */
class SpawnPoint2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpawnPoint2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] const Utf8String& GetSpawnName() const noexcept { return spawnName; }
    void SetSpawnName(const char* name) noexcept { spawnName = Utf8String(name != nullptr ? name : ""); }

    /** Planar facing in radians (world +X = 0). Used when <c>useTransformFacing</c> is false. */
    [[nodiscard]] float GetFacingRadians() const noexcept { return facingRadians; }
    void SetFacingRadians(const float radians) noexcept { facingRadians = radians; }

    /**
     * When true, <c>SpawnPoint2DService</c> derives facing from the owner's local Z rotation
     * instead of <c>facingRadians</c>.
     */
    [[nodiscard]] bool GetUseTransformFacing() const noexcept { return useTransformFacing; }
    void SetUseTransformFacing(const bool value) noexcept { useTransformFacing = value; }

    /** Optional gameplay tag (team, lane, etc.). */
    [[nodiscard]] int GetTeamId() const noexcept { return teamId; }
    void SetTeamId(const int id) noexcept { teamId = id; }

private:
    Utf8String spawnName{"Player"};
    float facingRadians = 0.0F;
    bool useTransformFacing = true;
    int teamId = 0;
};

}  // namespace Spark
