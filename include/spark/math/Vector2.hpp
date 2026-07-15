#pragma once

#include "spark/math/Constants.hpp"

#include <cmath>

namespace Spark {

struct Vector2 {
    float x = 0.0F;
    float y = 0.0F;

    static const Vector2 Zero;
    static const Vector2 One;
    static const Vector2 UnitX;
    static const Vector2 UnitY;

    constexpr Vector2() noexcept = default;
    constexpr Vector2(float inX, float inY) noexcept : x(inX), y(inY) {}

    [[nodiscard]] constexpr Vector2 operator+(const Vector2& v) const noexcept {
        return {x + v.x, y + v.y};
    }
    [[nodiscard]] constexpr Vector2 operator-(const Vector2& v) const noexcept {
        return {x - v.x, y - v.y};
    }
    [[nodiscard]] constexpr Vector2 operator*(float s) const noexcept { return {x * s, y * s}; }
    [[nodiscard]] constexpr Vector2 operator/(float s) const noexcept { return {x / s, y / s}; }

    constexpr Vector2& operator+=(const Vector2& v) noexcept {
        x += v.x;
        y += v.y;
        return *this;
    }
    constexpr Vector2& operator-=(const Vector2& v) noexcept {
        x -= v.x;
        y -= v.y;
        return *this;
    }
    constexpr Vector2& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        return *this;
    }
    constexpr Vector2& operator/=(float s) noexcept {
        x /= s;
        y /= s;
        return *this;
    }

    [[nodiscard]] constexpr float operator[](int i) const noexcept { return i == 0 ? x : y; }
    [[nodiscard]] constexpr float& operator[](int i) noexcept { return i == 0 ? x : y; }

    [[nodiscard]] float LengthSquared() const noexcept { return x * x + y * y; }
    [[nodiscard]] float Length() const noexcept { return std::sqrt(LengthSquared()); }

    [[nodiscard]] Vector2 Normalized() const noexcept {
        const float len = Length();
        if (len < Epsilon) {
            return Zero;
        }
        return *this / len;
    }

    [[nodiscard]] static constexpr float Dot(const Vector2& a, const Vector2& b) noexcept {
        return a.x * b.x + a.y * b.y;
    }

    [[nodiscard]] static Vector2 Lerp(const Vector2& a, const Vector2& b, float t) noexcept {
        return a * (1.0F - t) + b * t;
    }
};

inline constexpr Vector2 Vector2::Zero{0.0F, 0.0F};
inline constexpr Vector2 Vector2::One{1.0F, 1.0F};
inline constexpr Vector2 Vector2::UnitX{1.0F, 0.0F};
inline constexpr Vector2 Vector2::UnitY{0.0F, 1.0F};

[[nodiscard]] constexpr Vector2 operator*(float s, const Vector2& v) noexcept { return v * s; }

}  // namespace Spark
