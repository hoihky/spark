#pragma once

#include <cstdint>

namespace Spark {

class CombatPhaseKind final {
public:
    enum class Value : std::uint8_t { Idle = 0, Windup = 1, Active = 2, Recovery = 3, Complete = 4 };

    constexpr CombatPhaseKind() noexcept : value(Value::Idle) {}
    constexpr explicit CombatPhaseKind(const Value v) noexcept : value(v) {}

    [[nodiscard]] constexpr Value GetValue() const noexcept { return value; }

private:
    Value value;
};

}  // namespace Spark
