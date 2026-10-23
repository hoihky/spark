#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/gameplay/GameplayTeamId.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Assigns a <c>GameplayTeamId</c> to an object for combat and interaction rules. */
class GameplayTeamComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GameplayTeam;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(set_team_id)
    void SetTeamId(const std::int32_t id) noexcept { team = GameplayTeamId{id}; }
    SPARK_SCRIPT_BIND(get_team_id)
    [[nodiscard]] std::int32_t GetTeamId() const noexcept { return team.GetValue(); }

    SPARK_SCRIPT_BIND(set_gameplay_team)
    void SetGameplayTeam(const GameplayTeamId id) noexcept { team = id; }
    SPARK_SCRIPT_BIND(get_gameplay_team)
    [[nodiscard]] GameplayTeamId GetGameplayTeam() const noexcept { return team; }

private:
    GameplayTeamId team{};
};

}  // namespace Spark
