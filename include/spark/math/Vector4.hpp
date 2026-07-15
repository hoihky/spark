#pragma once

#include "spark/math/Constants.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

struct Vector4 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 0.0F;

    static const Vector4 Zero;
    static const Vector4 One;
    static const Vector4 UnitW;

    constexpr Vector4() noexcept = default;
    constexpr Vector4(float inX, float inY, float inZ, float inW) noexcept : x(inX), y(inY), z(inZ), w(inW) {}
    constexpr explicit Vector4(const Vector3& xyz, float inW) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(inW) {}

    [[nodiscard]] constexpr Vector3 ToVector3() const noexcept { return {x, y, z}; }

    [[nodiscard]] constexpr Vector4 operator+(const Vector4& v) const noexcept {
        return {x + v.x, y + v.y, z + v.z, w + v.w};
    }
    [[nodiscard]] constexpr Vector4 operator-(const Vector4& v) const noexcept {
        return {x - v.x, y - v.y, z - v.z, w - v.w};
    }
    [[nodiscard]] constexpr Vector4 operator*(float s) const noexcept {
        return {x * s, y * s, z * s, w * s};
    }
    [[nodiscard]] constexpr Vector4 operator/(float s) const noexcept {
        return {x / s, y / s, z / s, w / s};
    }

    constexpr Vector4& operator+=(const Vector4& v) noexcept {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }

    [[nodiscard]] constexpr float operator[](int i) const noexcept {
        return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
    }
    [[nodiscard]] constexpr float& operator[](int i) noexcept {
        return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
    }

    [[nodiscard]] float LengthSquared() const noexcept { return x * x + y * y + z * z + w * w; }
    [[nodiscard]] float Length() const noexcept { return std::sqrt(LengthSquared()); }

    [[nodiscard]] Vector4 Normalized() const noexcept {
        const float len = Length();
        if (len < Epsilon) {
            return Zero;
        }
        return *this / len;
    }

    [[nodiscard]] static constexpr float Dot(const Vector4& a, const Vector4& b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    [[nodiscard]] static Vector4 Lerp(const Vector4& a, const Vector4& b, float t) noexcept {
        return a * (1.0F - t) + b * t;
    }
};

inline constexpr Vector4 Vector4::Zero{0.0F, 0.0F, 0.0F, 0.0F};
inline constexpr Vector4 Vector4::One{1.0F, 1.0F, 1.0F, 1.0F};
inline constexpr Vector4 Vector4::UnitW{0.0F, 0.0F, 0.0F, 1.0F};

[[nodiscard]] constexpr Vector4 operator*(float s, const Vector4& v) noexcept { return v * s; }

}  // namespace Spark
