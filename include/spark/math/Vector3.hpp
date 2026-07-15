#pragma once

#include "spark/math/Constants.hpp"

#include <cmath>

namespace Spark {

struct Vector3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;

    static const Vector3 Zero;
    static const Vector3 One;
    static const Vector3 UnitX;
    static const Vector3 UnitY;
    static const Vector3 UnitZ;

    constexpr Vector3() noexcept = default;
    constexpr Vector3(float inX, float inY, float inZ) noexcept : x(inX), y(inY), z(inZ) {}

    [[nodiscard]] constexpr Vector3 operator+(const Vector3& v) const noexcept {
        return {x + v.x, y + v.y, z + v.z};
    }
    [[nodiscard]] constexpr Vector3 operator-(const Vector3& v) const noexcept {
        return {x - v.x, y - v.y, z - v.z};
    }
    [[nodiscard]] constexpr Vector3 operator-() const noexcept { return {-x, -y, -z}; }
    [[nodiscard]] constexpr Vector3 operator*(float s) const noexcept { return {x * s, y * s, z * s}; }
    [[nodiscard]] constexpr Vector3 operator/(float s) const noexcept { return {x / s, y / s, z / s}; }

    constexpr Vector3& operator+=(const Vector3& v) noexcept {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    constexpr Vector3& operator-=(const Vector3& v) noexcept {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    constexpr Vector3& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    constexpr Vector3& operator/=(float s) noexcept {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    [[nodiscard]] constexpr float operator[](int i) const noexcept {
        return i == 0 ? x : (i == 1 ? y : z);
    }
    [[nodiscard]] constexpr float& operator[](int i) noexcept { return i == 0 ? x : (i == 1 ? y : z); }

    [[nodiscard]] float LengthSquared() const noexcept { return x * x + y * y + z * z; }
    [[nodiscard]] float Length() const noexcept { return std::sqrt(LengthSquared()); }

    [[nodiscard]] Vector3 Normalized() const noexcept {
        const float len = Length();
        if (len < Epsilon) {
            return Zero;
        }
        return *this / len;
    }

    [[nodiscard]] static constexpr float Dot(const Vector3& a, const Vector3& b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    [[nodiscard]] static constexpr Vector3 Cross(const Vector3& a, const Vector3& b) noexcept {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    [[nodiscard]] static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) noexcept {
        return a * (1.0F - t) + b * t;
    }
};

inline constexpr Vector3 Vector3::Zero{0.0F, 0.0F, 0.0F};
inline constexpr Vector3 Vector3::One{1.0F, 1.0F, 1.0F};
inline constexpr Vector3 Vector3::UnitX{1.0F, 0.0F, 0.0F};
inline constexpr Vector3 Vector3::UnitY{0.0F, 1.0F, 0.0F};
inline constexpr Vector3 Vector3::UnitZ{0.0F, 0.0F, 1.0F};

[[nodiscard]] constexpr Vector3 operator*(float s, const Vector3& v) noexcept { return v * s; }

}  // namespace Spark
