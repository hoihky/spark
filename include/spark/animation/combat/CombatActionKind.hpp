#pragma once

#include <cstdint>

namespace Spark {

class CombatActionKind final {
public:
    enum class Value : std::uint8_t { None = 0, Hurt = 1, Attack = 2, Stagger = 3, Death = 4 };

    constexpr CombatActionKind() noexcept : value(Value::None) {}
    constexpr explicit CombatActionKind(const Value v) noexcept : value(v) {}

    [[nodiscard]] constexpr Value GetValue() const noexcept { return value; }
    [[nodiscard]] constexpr int ToInt() const noexcept { return static_cast<int>(value); }

    [[nodiscard]] static constexpr CombatActionKind FromInt(const int raw) noexcept {
        return CombatActionKind(static_cast<Value>(raw));
    }

private:
    Value value;
};

}  // namespace Spark
