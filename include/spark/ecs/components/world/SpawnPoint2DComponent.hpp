#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Named 2D spawn marker (position from transform, optional planar facing).
 * Resolved through <c>SpawnPoint2DService</c> / <c>FindSpawnPoint2D</c>.
 */
class SpawnPoint2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpawnPoint2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_spawn_name)
    [[nodiscard]] const Utf8String& GetSpawnName() const noexcept { return spawnName; }
    SPARK_SCRIPT_BIND(set_spawn_name)
    void SetSpawnName(const char* name) noexcept { spawnName = Utf8String(name != nullptr ? name : ""); }

    /** Planar facing in radians (world +X = 0). Used when <c>useTransformFacing</c> is false. */
    SPARK_SCRIPT_BIND(get_facing_radians)
    [[nodiscard]] float GetFacingRadians() const noexcept { return facingRadians; }
    SPARK_SCRIPT_BIND(set_facing_radians)
    void SetFacingRadians(const float radians) noexcept { facingRadians = radians; }

    /**
     * When true, <c>SpawnPoint2DService</c> derives facing from the owner's local Z rotation
     * instead of <c>facingRadians</c>.
     */
    SPARK_SCRIPT_BIND(get_use_transform_facing)
    [[nodiscard]] bool GetUseTransformFacing() const noexcept { return useTransformFacing; }
    SPARK_SCRIPT_BIND(set_use_transform_facing)
    void SetUseTransformFacing(const bool value) noexcept { useTransformFacing = value; }

    /** Optional gameplay tag (team, lane, etc.). */
    SPARK_SCRIPT_BIND(get_team_id)
    [[nodiscard]] int GetTeamId() const noexcept { return teamId; }
    SPARK_SCRIPT_BIND(set_team_id)
    void SetTeamId(const int id) noexcept { teamId = id; }

private:
    Utf8String spawnName{"Player"};
    float facingRadians = 0.0F;
    bool useTransformFacing = true;
    int teamId = 0;
};

}  // namespace Spark
