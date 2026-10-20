#pragma once

#include <cstdint>

namespace Spark {

/** Stable gameplay team identifier (0 = neutral / unassigned). */
class GameplayTeamId final {
public:
    constexpr GameplayTeamId() noexcept = default;
    constexpr explicit GameplayTeamId(const std::int32_t id) noexcept : teamId(id) {}

    [[nodiscard]] constexpr std::int32_t GetValue() const noexcept { return teamId; }
    [[nodiscard]] constexpr bool IsNeutral() const noexcept { return teamId == 0; }

    constexpr bool operator==(const GameplayTeamId other) const noexcept { return teamId == other.teamId; }
    constexpr bool operator!=(const GameplayTeamId other) const noexcept { return teamId != other.teamId; }

private:
    std::int32_t teamId = 0;
};

}  // namespace Spark
