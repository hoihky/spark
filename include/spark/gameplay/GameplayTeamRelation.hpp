#pragma once

#include <cstdint>

namespace Spark {

class GameplayTeamRelation final {
public:
    enum class Value : std::uint8_t { Self = 0, Ally = 1, Enemy = 2, Neutral = 3 };

    constexpr GameplayTeamRelation() noexcept = default;
    constexpr explicit GameplayTeamRelation(const Value v) noexcept : value(v) {}

    [[nodiscard]] constexpr Value GetValue() const noexcept { return value; }
    [[nodiscard]] constexpr bool IsHostile() const noexcept { return value == Value::Enemy; }

private:
    Value value = Value::Neutral;
};

}  // namespace Spark
